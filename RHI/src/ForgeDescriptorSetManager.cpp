#include "Forge.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeImage.h"
#include "ForgeShader.h"
#include "ForgeLogger.h"
#include "ForgeBindingList.h"
#include "ForgeUtils.h"
#include "ForgeDynamicMemory.h"
#include "ForgeBuffer.h"
#include "ForgeDeletionQueue.h"

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

	static void
	_forge_descriptor_set_manager_free(Forge* forge, ForgeDescriptorSetManager* manager)
	{
	}

	ForgeDescriptorSetManager*
	forge_descriptor_set_manager_new(Forge* forge)
	{
		auto manager = new ForgeDescriptorSetManager();

		return manager;
	}

	VkDescriptorSet
	forge_descriptor_set_acquire(Forge* forge, ForgeDescriptorSetManager* manager, VkDescriptorSetLayout layout, ForgeBindingList* binding_list)
	{
		return VK_NULL_HANDLE;
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