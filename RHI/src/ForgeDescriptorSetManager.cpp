#include "Forge.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeImage.h"
#include "ForgeShader.h"
#include "ForgeLogger.h"
#include "ForgeBindingList.h"
#include "ForgeUtils.h"
#include "ForgeDynamicMemory.h"
#include "ForgeBuffer.h"

namespace forge
{
	static uint64_t
	_forge_bindings_hash(const ForgeBindingList& bindings)
	{
		uint64_t seed = 0u;

		for (auto& image : bindings.images)
		{
			if (image == nullptr)
				continue;

			_forge_hash_combine(seed, image->handle);
			_forge_hash_combine(seed, image->memory);
		}

		return seed;
	}

	static void
	_forge_descriptor_set_update(Forge* forge, VkDescriptorSet set, const ForgeShaderDescription& shader_description, ForgeBindingList* binding_list)
	{
		VkWriteDescriptorSet MAX_WRITES[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS + FORGE_MAX_IMAGE_BINDINGS] = {};
		VkDescriptorBufferInfo MAX_BUFFERS[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS] = {};
		VkDescriptorImageInfo MAX_IMAGES[FORGE_MAX_IMAGE_BINDINGS] = {};
		uint32_t buffers_count = 0u;
		uint32_t images_count = 0u;

		auto& uniforms = shader_description.uniforms;
		for (uint32_t i = 0; i < FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS; ++i)
		{
			if (uniforms[i].name.empty() == true)
				continue;

			auto& buffer_write_info = MAX_BUFFERS[buffers_count];
			buffer_write_info.buffer = forge->uniform_memory->buffer->handle;
			buffer_write_info.offset = 0u;
			buffer_write_info.range  = uniforms[i].size;

			auto& write_info = MAX_WRITES[buffers_count];
			write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_info.dstSet = set;
			write_info.dstBinding = i;
			write_info.dstArrayElement = 0u;
			write_info.descriptorCount = 1u;
			write_info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			write_info.pBufferInfo = &buffer_write_info;

			++buffers_count;
		}

		auto& images = shader_description.images;
		for (uint32_t i = 0; i < FORGE_MAX_IMAGE_BINDINGS; ++i)
		{
			auto& image_layout = images[i];
			if (image_layout.name.empty() == true)
				continue;

			auto& image_write_info = MAX_IMAGES[images_count];
			image_write_info.imageLayout = image_layout.storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_write_info.imageView = binding_list->images[i]->shader_view;
			image_write_info.sampler = image_layout.storage ? VK_NULL_HANDLE : binding_list->images[i]->sampler;

			auto& write_info = MAX_WRITES[images_count + buffers_count];
			write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_info.dstSet = set;
			write_info.dstBinding = i;
			write_info.dstArrayElement = 0u;
			write_info.descriptorCount = 1u;
			write_info.descriptorType = image_layout.storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_info.pImageInfo = &image_write_info;

			++images_count;
		}

		vkUpdateDescriptorSets(forge->device, images_count + buffers_count, MAX_WRITES, 0u, nullptr);
	}

	static void
	_forge_descriptor_set_manager_free(Forge* forge, ForgeDescriptorSetManager* manager)
	{
		if (manager->pool)
		{
			vkDestroyDescriptorPool(forge->device, manager->pool, nullptr);
		}
	}

	ForgeDescriptorSetManager*
	forge_descriptor_set_manager_new(Forge* forge)
	{
		auto manager = new ForgeDescriptorSetManager();

		VkDescriptorPoolSize pool_sizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, FORGE_DESCRIPTOR_SET_MANAGER_MAX_DYNAMIC_UNIFORM_BUFFERS},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FORGE_DESCRIPTOR_SET_MANAGER_MAX_SAMPLED_IMAGES},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, FORGE_DESCRIPTOR_SET_MANAGER_MAX_STORAGE_IMAGES}
		};

		VkDescriptorPoolCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = FORGE_DESCRIPTOR_SET_MANAGER_MAX_DESCRIPTOR_SETS;
		info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
		info.pPoolSizes = pool_sizes;
		auto res = vkCreateDescriptorPool(forge->device, &info, nullptr, &manager->pool);
		VK_RES_CHECK(res);

		return manager;
	}

	VkDescriptorSet
	forge_descriptor_set_acquire(Forge* forge, ForgeDescriptorSetManager* manager, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		auto bindings_hash = _forge_bindings_hash(*binding_list);
		auto layout = shader->descriptor_set_layout;

		uint64_t value;
		auto res = vkGetSemaphoreCounterValue(forge->device, forge->timeline, &value);
		VK_RES_CHECK(res);

		for (auto& set : manager->allocated_sets)
		{
			if (set.layout == layout && set.active_bindings_hash == bindings_hash)
			{
				set.release_signal = forge->timeline_next_signal;

				return set.handle;
			}
		}

		for (auto& set : manager->allocated_sets)
		{
			if (set.layout == layout && (value >= set.release_signal && value - set.release_signal >= FORGE_DESCRIPTOR_SET_MANAGER_SET_MAX_AGE))
			{
				_forge_descriptor_set_update(forge, set.handle, shader->description, binding_list);

				set.active_bindings_hash = bindings_hash;
				set.release_signal = forge->timeline_next_signal;

				return set.handle;
			}
		}

		ForgeDescriptorSet set {};
		set.active_bindings_hash = bindings_hash;
		set.layout = layout;
		set.release_signal = forge->timeline_next_signal;

		VkDescriptorSetAllocateInfo allocate_info {};
		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.descriptorSetCount = 1u;
		allocate_info.pSetLayouts = &shader->descriptor_set_layout;
		allocate_info.descriptorPool = manager->pool;
		res = vkAllocateDescriptorSets(forge->device, &allocate_info, &set.handle);
		VK_RES_CHECK(res);

		_forge_descriptor_set_update(forge, set.handle, shader->description, binding_list);

		manager->allocated_sets.push_back(set);

		return set.handle;
	}

	void
	forge_descriptor_set_manager_destroy(Forge* forge, ForgeDescriptorSetManager* manager)
	{
		if (manager)
		{
			_forge_descriptor_set_manager_free(forge, manager);
			delete manager;
		}
	}
};