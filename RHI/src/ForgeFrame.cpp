#include "Forge.h"
#include "ForgeFrame.h"
#include "ForgeSwapchain.h"
#include "ForgeRenderPass.h"
#include "ForgeDeferedQueue.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeDynamicMemory.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeImage.h"

namespace forge
{
	static bool
	_forge_frame_pass_init(Forge* forge, ForgeFrame* frame, ForgeImage* color, ForgeImage* depth)
	{
		auto color_final_layout = frame->swapchain ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		ForgeAttachmentDescription color_attachment_desc {};
		color_attachment_desc.image = color;
		color_attachment_desc.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR; // TODO: This should be provided by the user
		color_attachment_desc.store_op = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_desc.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_desc.final_layout = color_final_layout;
		color_attachment_desc.clear_action.color[0] = 1.0f;
		color_attachment_desc.clear_action.color[1] = 0.0f;
		color_attachment_desc.clear_action.color[2] = 1.0f;
		color_attachment_desc.clear_action.color[3] = 1.0f;

		ForgeAttachmentDescription depth_attachment_desc {};
		depth_attachment_desc.image = depth;
		depth_attachment_desc.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment_desc.store_op = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment_desc.initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attachment_desc.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depth_attachment_desc.clear_action.depth = 1.0f;

		ForgeRenderPassDescription pass_desc {};
		pass_desc.colors[0] = color_attachment_desc;
		pass_desc.depth = depth_attachment_desc;
		frame->pass = forge_render_pass_new(forge, pass_desc);

		if (frame->pass == nullptr)
		{
			log_error("Failed to initialize frame's pass");
			return false;
		}

		return true;
	}

	static bool
	_forge_swapchain_frame_pass_init(Forge* forge, ForgeFrame* frame)
	{
		auto swapchain_desc = frame->swapchain->description;

		ForgeImageDescription color_desc {};
		color_desc.name = "Frame Color";
		color_desc.extent = {swapchain_desc.extent.width, swapchain_desc.extent.height, 1};
		color_desc.type = VK_IMAGE_TYPE_2D;
		color_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
		color_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		color_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		color_desc.create_flags = 0u;
		auto color = forge_image_new(forge, color_desc);

		ForgeImageDescription depth_desc{};
		depth_desc.name = "Frame Depth";
		depth_desc.extent = {swapchain_desc.extent.width, swapchain_desc.extent.height, 1};
		depth_desc.type = VK_IMAGE_TYPE_2D;
		depth_desc.format = VK_FORMAT_D32_SFLOAT;
		depth_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		depth_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depth_desc.create_flags = 0u;
		auto depth = forge_image_new(forge, depth_desc);

		_forge_frame_pass_init(forge, frame, color, depth);

		return true;
	}

	static void
	_forge_swapchain_frame_pass_update(Forge* forge, ForgeFrame* frame)
	{
		auto swapchain = frame->swapchain;
		auto pass = frame->pass;

		if (pass->width == swapchain->description.extent.width && pass->height == swapchain->description.extent.height)
		{
			return;
		}

		auto color = pass->description.colors[0].image;
		auto depth = pass->description.depth.image;

		forge_deferred_queue_push(forge, frame->deferred_queue, [forge, color, depth]() {
			forge_image_destroy(forge, color);
			forge_image_destroy(forge, depth);
		}, frame->current_frame + 1u);

		_forge_swapchain_frame_pass_init(forge, frame);
	}

	static bool
	_forge_offscreen_frame_pass_init(Forge* forge, ForgeFrame* frame, ForgeImage* color, ForgeImage* depth)
	{
		return _forge_frame_pass_init(forge, frame, color, depth);
	}

	static bool
	_forge_frame_common_init(Forge* forge, ForgeFrame* frame)
	{
		frame->deferred_queue = forge_deferred_queue_new(forge);

		if (frame->deferred_queue == nullptr)
		{
			log_error("Failed to initialize frame's deferred queue");
			return false;
		}

		frame->descriptor_set_manager = forge_descriptor_set_manager_new(forge);

		if (frame->descriptor_set_manager == nullptr)
		{
			log_error("Failed to initialize frame's descriptor set manager");
			return false;
		}

		frame->uniform_memory = forge_dynamic_memory_new(forge, FORGE_FRAME_MAX_UNIFORM_MEMORY, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

		if (frame->uniform_memory == nullptr)
		{
			log_error("Failed to initialize frame's uniform dynamic memory");
			return false;
		}

		VkSemaphoreTypeCreateInfo timeline_info {};
		timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timeline_info.initialValue = 0;

		VkSemaphoreCreateInfo rendering_done_info{};
		rendering_done_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		rendering_done_info.pNext = &timeline_info;
		rendering_done_info.flags = 0;
		auto res = vkCreateSemaphore(forge->device, &rendering_done_info, NULL, &frame->frame_finished);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize frame's rendering done semaphore");
			return false;
		}

		VkCommandPoolCreateInfo command_pool_info {};
		command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_info.queueFamilyIndex = forge->queue_family_index;
		res = vkCreateCommandPool(forge->device, &command_pool_info, nullptr, &frame->command_pool);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize frame's command pool");
			return false;
		}

		frame->current_frame = 0u;

		return true;
	}

	static bool
	_forge_swapchain_frame_init(Forge* forge, ForgeSwapchainDescription swapchain_desc, ForgeFrame* frame)
	{
		frame->swapchain = forge_swapchain_new(forge, swapchain_desc);

		if (frame->swapchain == nullptr)
		{
			log_error("Failed to initialize frame's swapchain");
			return false;
		}

		if (_forge_swapchain_frame_pass_init(forge, frame) == false)
		{
			return false;
		}

		if (_forge_frame_common_init(forge, frame) == false)
		{
			return false;
		}

		return true;
	}

	static bool
	_forge_offscreen_frame_init(Forge* forge, ForgeImage* color, ForgeImage* depth, ForgeFrame* frame)
	{
		if (_forge_offscreen_frame_pass_init(forge, frame, color, depth) == false)
		{
			return false;
		}

		if (_forge_frame_common_init(forge, frame) == false)
		{
			return false;
		}

		return true;
	}

	static void
	_forge_frame_free(Forge* forge, ForgeFrame* frame)
	{
		if (frame->swapchain)
		{
			forge_image_destroy(forge, frame->pass->description.colors[0].image);
			forge_image_destroy(forge, frame->pass->description.depth.image);
			forge_swapchain_destroy(forge, frame->swapchain);
		}

		vkDestroyCommandPool(forge->device, frame->command_pool, nullptr);
		vkDestroySemaphore(forge->device, frame->frame_finished, nullptr);
		forge_dynamic_memory_destroy(forge, frame->uniform_memory);
		forge_descriptor_set_manager_destroy(forge, frame->descriptor_set_manager);
		forge_deferred_queue_destroy(forge, frame->deferred_queue);
		forge_render_pass_destroy(forge, frame->pass);
	}

	static void
	_forge_pass_swapchain_blit(Forge* forge, ForgeFrame* frame)
	{
		VkResult res;

		auto swapchain = frame->swapchain;
		auto command_buffer = frame->command_buffer;
		auto image_available = swapchain->image_available[swapchain->frame_index % FORGE_SWAPCHIAN_INFLIGH_FRAMES];
		auto rendering_done = swapchain->rendering_done[swapchain->frame_index % FORGE_SWAPCHIAN_INFLIGH_FRAMES];
		auto fence = swapchain->fence[swapchain->frame_index % FORGE_SWAPCHIAN_INFLIGH_FRAMES];

		res = vkWaitForFences(forge->device, 1u, &fence, VK_TRUE, UINT64_MAX);
		VK_RES_CHECK(res);

		res = vkResetFences(forge->device, 1u, &fence);
		VK_RES_CHECK(res);

		res = vkAcquireNextImageKHR(forge->device, swapchain->handle, UINT64_MAX, image_available, VK_NULL_HANDLE, &swapchain->image_index);
		VK_RES_CHECK(res);

		auto dst_image = swapchain->images[swapchain->image_index];
		auto src_image = frame->pass->description.colors[0].image->handle;
		auto extent = swapchain->description.extent;

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.image = dst_image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0u;
		barrier.subresourceRange.levelCount = 1u;
		barrier.subresourceRange.baseArrayLayer = 0u;
		barrier.subresourceRange.layerCount = 1u;
		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);

		VkImageBlit image_blit {};
		image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_blit.srcSubresource.baseArrayLayer = 0u;
		image_blit.srcSubresource.layerCount = 1u;
		image_blit.srcSubresource.mipLevel = 0u;
		image_blit.srcOffsets[0] = {0,0,0};
		image_blit.srcOffsets[1] = {(int32_t)extent.width, (int32_t)extent.height, 1};
		image_blit.dstSubresource = image_blit.srcSubresource;
		image_blit.dstOffsets[0] = image_blit.srcOffsets[0];
		image_blit.dstOffsets[1] = image_blit.srcOffsets[1];
		vkCmdBlitImage(command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &image_blit, VK_FILTER_NEAREST);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_NONE;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.image = dst_image;
		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0u,
			0u, nullptr,
			0u, nullptr,
			1u, &barrier
		);
	}

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeImage* color, ForgeImage* depth)
	{
		auto frame = new ForgeFrame();
		forge->offscreen_frames[forge->offscreen_frames_count++] = frame;

		if (_forge_offscreen_frame_init(forge, color, depth, frame) == false)
		{
			forge_frame_destroy(forge, frame);
			return nullptr;
		}

		return frame;
	}

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeSwapchainDescription swapchain_desc)
	{
		auto frame = new ForgeFrame();
		forge->swapchain_frame = frame;

		if (_forge_swapchain_frame_init(forge, swapchain_desc, frame) == false)
		{
			forge_frame_destroy(forge, frame);
			return nullptr;
		}

		return frame;
	}

	bool
	forge_frame_begin(Forge* forge, ForgeFrame* frame)
	{
		VkResult res;

		auto swapchain = frame->swapchain;
		auto pass = frame->pass;

		// TODO: Add command buffer manager similar to descriptor set manager
		VkCommandBufferAllocateInfo command_info {};
		command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_info.commandBufferCount = 1u;
		command_info.commandPool = frame->command_pool;
		command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		res = vkAllocateCommandBuffers(forge->device, &command_info, &frame->command_buffer);
		VK_RES_CHECK(res);

		VkCommandBufferBeginInfo begin_info {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		res = vkBeginCommandBuffer(frame->command_buffer, &begin_info);
		VK_RES_CHECK(res);

		if (frame->swapchain)
		{
			forge_swapchain_update(forge, swapchain);
			_forge_swapchain_frame_pass_update(forge, frame);
			forge_render_pass_begin(forge, frame->command_buffer, pass);
		}
		else
		{
			forge_render_pass_begin(forge, frame->command_buffer, pass);
		}

		return true;
	}

	void
	forge_frame_end(Forge* forge, ForgeFrame* frame)
	{
		VkResult res;

		auto command_buffer = frame->command_buffer;
		auto swapchain = frame->swapchain;

		forge_render_pass_end(forge, command_buffer, frame->pass);

		if (swapchain)
		{
			_forge_pass_swapchain_blit(forge, frame);
		}

		vkEndCommandBuffer(command_buffer);
	}

	void
	forge_frame_deferred_task_add(Forge* forge, ForgeFrame* frame, const std::function<void()>& task)
	{
		forge_deferred_queue_push(forge, frame->deferred_queue, task, frame->current_frame + 1);
	}

	void
	forge_frame_destroy(Forge* forge, ForgeFrame* frame)
	{
		if (frame)
		{
			_forge_frame_free(forge, frame);
			delete frame;
		}
	}
};