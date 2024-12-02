#include "Forge.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeImage.h"
#include "ForgeShader.h"
#include "ForgeLogger.h"
#include "ForgeBindingList.h"
#include "ForgeUtils.h"

namespace forge
{
	static void
	_forge_descriptor_set_update(Forge* forge, VkDescriptorSet set, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		VkWriteDescriptorSet MAX_WRITES[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS + FORGE_MAX_IMAGE_BINDINGS] = {};
		VkDescriptorBufferInfo MAX_BUFFERS[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS] = {};
		VkDescriptorImageInfo MAX_IMAGES[FORGE_MAX_IMAGE_BINDINGS] = {};
		uint32_t buffers_count = 0u;
		uint32_t images_count = 0u;

		auto& uniforms = shader->description.uniforms;
		for (uint32_t i = 0; i < FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS; ++i)
		{
			if (uniforms[i].name.empty() == true)
				continue;

			auto& buffer_write_info = MAX_BUFFERS[buffers_count];
			buffer_write_info.buffer = VK_NULL_HANDLE; // TODO: ForgeDynamicMemory
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

		auto& images = shader->description.images;
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

	static bool
	_forge_descriptor_set_allocator_init(Forge* forge, ForgeShader* shader, ForgeDescriptorSetAllocator* allocator)
	{
		uint32_t uniforms_count = 0u;
		uint32_t sampled_images_count = 0u;
		uint32_t storage_images_count = 0u;

		for (auto uniform : shader->description.uniforms)
		{
			if (uniform.name.empty() == true)
				continue;

			++uniforms_count;
		}

		for (auto image : shader->description.images)
		{
			if (image.name.empty() == true)
				continue;

			if (image.storage)
			{
				++storage_images_count;
			}
			else
			{
				++sampled_images_count;
			}
		}

		std::vector<VkDescriptorPoolSize> pool_sizes;

		if (uniforms_count > 0u)
		{
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, uniforms_count * FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS});
		}

		if (sampled_images_count > 0u)
		{
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sampled_images_count * FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS});
		}

		if (storage_images_count > 0u)
		{
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storage_images_count * FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS});
		}

		VkDescriptorPoolCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS;
		info.poolSizeCount = (uint32_t)pool_sizes.size();
		info.pPoolSizes = pool_sizes.data();
		auto res = vkCreateDescriptorPool(forge->device, &info, nullptr, &allocator->pool);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize descriptor set allocator");
			return false;
		}

		return true;
	}

	static VkDescriptorSet
	_forge_descriptor_set_allocator_set_get(Forge* forge, ForgeDescriptorSetAllocator* allocator, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		uint64_t hash = 0;

		// Generate hash for bindings that changing them could invoke a vkDescriptorSetUpdate

		for (auto image : binding_list->images)
		{
			if (image == nullptr)
				continue;

			_forge_hash_combine(hash, image);
			_forge_hash_combine(hash, image->handle);
		}

		if (auto iter = allocator->recently_used.find(hash); iter != allocator->recently_used.end())
		{
			iter->second.last_use = 0u; // TODO: set to the current frame index
			return iter->second.handle;
		}
		else
		{
			VkDescriptorSet set {};

			if (allocator->available.size() > 0)
			{
				set = allocator->available.back();
				allocator->available.pop_back();

				return set;
			}
			else
			{
				assert(allocator->allocatoed_sets < FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS);

				VkDescriptorSetAllocateInfo allocate_info {};
				allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocate_info.descriptorSetCount = 1u;
				allocate_info.pSetLayouts = &shader->descriptor_set_layout;
				allocate_info.descriptorPool = allocator->pool;
				auto res = vkAllocateDescriptorSets(forge->device, &allocate_info, &set);
				VK_RES_CHECK(res);

				++allocator->allocatoed_sets;
			}

			allocator->recently_used[hash] = {set, 0u}; // TODO: set to the current frame index
			_forge_descriptor_set_update(forge, set, shader, binding_list);

			return set;
		}
	}

	static void
	_forge_descriptor_set_manager_free(Forge* forge, ForgeDescriptorSetManager* manager)
	{
		for (auto [layout, allocator] : manager->allocators)
			vkDestroyDescriptorPool(forge->device, allocator.pool, nullptr);
	}

	ForgeDescriptorSetManager*
	forge_descriptor_set_manager_new(Forge* forge)
	{
		auto manager = new ForgeDescriptorSetManager();

		return manager;
	}

	VkDescriptorSet
	forge_descriptor_set_manager_set_get(Forge* forge, ForgeDescriptorSetManager* manager, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		if (auto iter = manager->allocators.find(shader->descriptor_set_layout); iter != manager->allocators.end())
		{
			return _forge_descriptor_set_allocator_set_get(forge, &iter->second, shader, binding_list);
		}
		else
		{
			manager->allocators[shader->descriptor_set_layout] = {};
			_forge_descriptor_set_allocator_init(forge, shader, &manager->allocators[shader->descriptor_set_layout]);
			return _forge_descriptor_set_allocator_set_get(forge, &manager->allocators[shader->descriptor_set_layout], shader, binding_list);
		}
	}

	void
	forge_descriptor_set_manager_flush(Forge* forge, ForgeDescriptorSetManager* manager)
	{
	
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