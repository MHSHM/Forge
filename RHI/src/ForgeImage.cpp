#include "ForgeImage.h"
#include "Forge.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeBuffer.h"
#include "ForgeDeletionQueue.h"

namespace forge
{
	static bool
	_forge_image_init(Forge* forge, ForgeImage* image)
	{
		VkResult res;

		bool is_cube_map = image->description.create_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		if (is_cube_map)
		{
			if (image->description.extent.width != image->description.extent.height)
			{
				log_error("Cubemaps should have identical width and height");
				return false;
			}
		}

		uint32_t levels_count = 1u;
		if (image->description.mipmaps == true)
		{
			uint32_t largest_dimension = std::max({ image->description.extent.width, image->description.extent.height, 1u });
			levels_count = static_cast<uint32_t>(std::floor(std::log2(largest_dimension))) + 1;
		}

		image->aspect = _forge_image_aspect(image->description.format);
		image->layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageCreateInfo image_info{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.flags = image->description.create_flags;
		image_info.imageType = image->description.type;
		image_info.extent = image->description.extent;
		image_info.mipLevels = levels_count;
		image_info.arrayLayers = is_cube_map ? 6u : 1;
		image_info.format = image->description.format;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = image->description.usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		res = vkCreateImage(forge->device, &image_info, nullptr, &image->handle);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create image '{}'", image->description.name);
			return false;
		}

		VkMemoryRequirements mem_requirements;
		vkGetImageMemoryRequirements(forge->device, image->handle, &mem_requirements);

		VkMemoryAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = _find_memory_type(forge, mem_requirements.memoryTypeBits, image->description.memory_properties);

		res = vkAllocateMemory(forge->device, &alloc_info, nullptr, &image->memory);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to allocate memory for image '{}'", image->description.name);
			return false;
		}

		res = vkBindImageMemory(forge->device, image->handle, image->memory, 0);
		VK_RES_CHECK(res);

		// shader view
		VkImageViewCreateInfo view_info {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = image->handle;
		view_info.viewType = is_cube_map ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = image->description.format;
		view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.subresourceRange.aspectMask = image->aspect;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = levels_count;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = is_cube_map ? 6u : 1;
		res = vkCreateImageView(forge->device, &view_info, nullptr, &image->shader_view);
		VK_RES_CHECK(res);

		if ((image->description.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) || (image->description.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			// render target view
			view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_info.subresourceRange.levelCount = 1u;
			view_info.subresourceRange.layerCount = 1u;
			res = vkCreateImageView(forge->device, &view_info, nullptr, &image->render_target_view);
			VK_RES_CHECK(res);
		}

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create image view for '{}'", image->description.name);
			return false;
		}

		if ((image->description.usage & VK_IMAGE_USAGE_STORAGE_BIT) == 0)
		{
			VkSamplerCreateInfo sampler_info{};
			sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler_info.magFilter = image->description.mag_filter;
			sampler_info.minFilter = image->description.min_filter;
			sampler_info.mipmapMode = image->description.mipmap_mode;
			sampler_info.addressModeU = image->description.address_mode_u;
			sampler_info.addressModeV = image->description.address_mode_v;
			sampler_info.addressModeW = image->description.address_mode_w;
			sampler_info.mipLodBias = 0.0f;
			sampler_info.anisotropyEnable = VK_FALSE;
			sampler_info.compareEnable = VK_FALSE;
			sampler_info.minLod = 0.0f;
			sampler_info.maxLod = VK_LOD_CLAMP_NONE;
			sampler_info.unnormalizedCoordinates = VK_FALSE;
			res = vkCreateSampler(forge->device, &sampler_info, nullptr, &image->sampler);
			VK_RES_CHECK(res);

			if (res != VK_SUCCESS)
			{
				log_error("Failed to create the sampler for '{}'", image->description.name);
				return false;
			}
		}

		if (image->description.name.empty() == false)
		{
			_forge_debug_obj_name_set(forge, (uint64_t)image->handle, VK_OBJECT_TYPE_IMAGE, image->description.name.c_str());
		}

		log_info("Image '{}' initialized successfully", image->description.name);
		image->view_type = is_cube_map ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

		return true;
	}

	static void
	_forge_image_free(Forge* forge, ForgeImage* image)
	{
		if (image->sampler)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, image->sampler);
		}

		if (image->shader_view)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, image->shader_view);
		}

		if (image->render_target_view)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, image->render_target_view);
		}

		if (image->memory)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, image->memory);
		}

		if (image->handle)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, image->handle);
		}
	}

	static void
	_forge_image_layout_transition(Forge* forge, VkCommandBuffer command_buffer, VkImageLayout new_layout, ForgeImage* image)
	{
		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = _forge_image_memory_barrier_src_access(image->layout);
		barrier.dstAccessMask = _forge_image_memory_barrier_dst_access(new_layout);
		barrier.oldLayout = image->layout;
		barrier.newLayout = new_layout;
		barrier.image = image->handle;
		barrier.subresourceRange.aspectMask = image->aspect;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0u;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		vkCmdPipelineBarrier(
			command_buffer,
			_forge_image_memory_barrier_pipeline_stage(image->layout),
			_forge_image_memory_barrier_pipeline_stage(new_layout),
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);

		image->layout = new_layout;
	}

	static void
	_forge_image_write(Forge* forge, ForgeImage* image, uint32_t layer, uint32_t size, void* data)
	{
		VkResult res;

		auto remaining_size = size;
		auto staging_buffer = forge->staging_buffer;
		auto staging_command_buffer = forge->staging_command_buffer;
		auto format_size = _forge_format_size(image->description.format);
		auto row_size = image->description.extent.width * format_size;
		auto count = 0u;

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		res = vkBeginCommandBuffer(staging_command_buffer, &begin_info);
		VK_RES_CHECK(res);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.image = image->handle;
		barrier.subresourceRange.aspectMask = image->aspect;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.levelCount = 1u;
		barrier.subresourceRange.baseArrayLayer = layer;
		barrier.subresourceRange.layerCount = 1u;
		vkCmdPipelineBarrier(
			staging_command_buffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);

		while (remaining_size > 0)
		{
			auto write_size = remaining_size;
			auto written_size = size - remaining_size;
			bool reset = (staging_buffer->cursor + write_size > staging_buffer->description.size) || (staging_buffer->cursor % format_size != 0);

			if (reset)
			{
				staging_buffer->cursor = 0;

				vkEndCommandBuffer(staging_command_buffer);

				// Submit pending transfer operations
				VkSubmitInfo submit_info{};
				submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.commandBufferCount = 1;
				submit_info.pCommandBuffers = &staging_command_buffer;
				res = vkQueueSubmit(forge->queue, 1u, &submit_info, VK_NULL_HANDLE);
				VK_RES_CHECK(res);

				// Wait for the queue to ensure all transfers are complete
				// TODO: We shouldn't halt the whole queue for this
				vkQueueWaitIdle(forge->queue);

				res = vkBeginCommandBuffer(staging_command_buffer, &begin_info);
				VK_RES_CHECK(res);
			}

			if (staging_buffer->cursor + write_size > staging_buffer->description.size)
			{
				write_size = staging_buffer->description.size;
			}

			write_size -= (write_size % row_size);

			memcpy((char*)staging_buffer->mapped_ptr + staging_buffer->cursor, (char*)data + written_size, write_size);

			VkBufferImageCopy copy_info{};
			copy_info.bufferOffset = staging_buffer->cursor;
			copy_info.imageSubresource.aspectMask = image->aspect;
			copy_info.imageSubresource.mipLevel = 0u;
			copy_info.imageSubresource.baseArrayLayer = layer;
			copy_info.imageSubresource.layerCount = 1u;
			copy_info.imageOffset.x = 0u;
			copy_info.imageOffset.y = written_size / row_size;
			copy_info.imageExtent.width = image->description.extent.width;
			copy_info.imageExtent.height = std::max(write_size / row_size, 1u);
			copy_info.imageExtent.depth = 1u;
			vkCmdCopyBufferToImage(staging_command_buffer, staging_buffer->handle, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy_info);

			staging_buffer->cursor += write_size;
			remaining_size -= write_size;
			count += 1u;
		}

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(
			staging_command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);

		vkEndCommandBuffer(forge->staging_command_buffer);

		log_info("Writing operation to '{}' was done using '{}' copy operations (Staging buffer size: {}, Total data size: {})",
			image->description.name, count, staging_buffer->description.size, size);
	}

	static void
	_forge_image_mipmaps_generate(Forge* forge, VkCommandBuffer command_buffer, ForgeImage* image)
	{
		uint32_t layers_count = image->description.create_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? 6u : 1;
		uint32_t largest_dimension = std::max({ image->description.extent.width, image->description.extent.height, 1u });
		uint32_t levels_count = static_cast<uint32_t>(std::floor(std::log2(largest_dimension))) + 1;

		_forge_image_layout_transition(forge, command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image);

		int32_t mip_width = image->description.extent.width;
		int32_t mip_height = image->description.extent.height;

		for (uint32_t i = 0; i < levels_count - 1; ++i)
		{
			VkImageMemoryBarrier barrier {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.image = image->handle;
			barrier.subresourceRange.aspectMask = image->aspect;
			barrier.subresourceRange.baseMipLevel = i;
			barrier.subresourceRange.levelCount = 1u;
			barrier.subresourceRange.baseArrayLayer = 0u;
			barrier.subresourceRange.layerCount = layers_count;
			vkCmdPipelineBarrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0u,
				0u, nullptr,
				0u, nullptr,
				1u, &barrier
			);

			VkImageBlit blit {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mip_width, mip_height, 1 };
			blit.srcSubresource.aspectMask = image->aspect;
			blit.srcSubresource.mipLevel = i;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = layers_count;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { std::max(mip_width / 2, 1), std::max(mip_height / 2, 1), 1 };
			blit.dstSubresource.aspectMask = image->aspect;
			blit.dstSubresource.mipLevel = i + 1;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = layers_count;

			vkCmdBlitImage(
				command_buffer,
				image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR
			);

			mip_width = std::max(mip_width / 2u, 1u);
			mip_height = std::max(mip_height / 2u, 1u);
		}

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.image = image->handle;
		barrier.subresourceRange.aspectMask = image->aspect;
		barrier.subresourceRange.baseMipLevel = levels_count - 1u;
		barrier.subresourceRange.levelCount = 1u;
		barrier.subresourceRange.baseArrayLayer = 0u;
		barrier.subresourceRange.layerCount = layers_count;
		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);

		barrier.srcAccessMask = 0u;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.levelCount = levels_count;
		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);

		log_info("Mipmaps for '{}' were generated successfully with '{}' levels (Largest Dimension: '{}')", image->description.name, levels_count, largest_dimension);
	}

	ForgeImage*
	forge_image_new(Forge* forge, ForgeImageDescription descriptrion)
	{
		auto image = new ForgeImage;
		image->description = descriptrion;

		if (!_forge_image_init(forge, image))
		{
			forge_image_destroy(forge, image);
			return nullptr;
		}

		return image;
	}

	void
	forge_image_destroy(Forge* forge, ForgeImage* image)
	{
		if (image)
		{
			_forge_image_free(forge, image);
			delete image;
		}
	}

	void
	forge_image_write(Forge* forge, ForgeImage* image, uint32_t layer, uint32_t size, void* data)
	{
		auto expected_size = image->description.extent.width * image->description.extent.height * _forge_format_size(image->description.format);

		if (size != expected_size) { log_error("Size doesn't match the expected size based on the image format"); return; }
		if (layer > 1 && ((image->description.create_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) == 0)) { log_error("Layer can't be larger than '1' if the image is not a cubemaps"); return; }
		if (layer > 6) { log_error("Layer can't be larger than 6 (cube faces)"); return; }

		_forge_image_write(forge, image, layer, size, data);
	}

	void
	forge_image_mipmaps_generate(Forge* forge, VkCommandBuffer command_buffer, ForgeImage* image)
	{
		if (image->description.mipmaps == false) { log_warning("Image was not marked as it should have mipmaps, skipping the operation"); return; }

		_forge_image_mipmaps_generate(forge, command_buffer, image);
	}

	void
	forge_image_layout_transition(Forge* forge, VkCommandBuffer command_buffer, VkImageLayout new_layout, ForgeImage* image)
	{
		if (image->layout == new_layout){return;}

		_forge_image_layout_transition(forge, command_buffer, new_layout, image);
	}
}