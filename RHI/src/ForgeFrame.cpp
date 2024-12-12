#include "Forge.h"
#include "ForgeFrame.h"
#include "ForgeSwapchain.h"
#include "ForgeRenderPass.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeImage.h"
#include "ForgeBuffer.h"
#include "ForgeShader.h"
#include "ForgeBindingList.h"
#include "ForgeCommandBufferManager.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeDynamicMemory.h"
#include "ForgeDeletionQueue.h"

namespace forge
{
	static void
	_forge_frame_pass_update(Forge* forge, ForgeFrame* frame, uint32_t width, uint32_t height)
	{
		auto pass = frame->pass;

		if (pass && pass->width == width && pass->height == height)
		{
			return;
		}

		ForgeImageDescription color_desc{};
		color_desc.name = "Frame Color";
		color_desc.extent = {width, height, 1};
		color_desc.type = VK_IMAGE_TYPE_2D;
		color_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
		color_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		color_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		color_desc.create_flags = 0u;
		auto color = forge_image_new(forge, color_desc);

		ForgeImageDescription depth_desc{};
		depth_desc.name = "Frame Depth";
		depth_desc.extent = {width, height, 1};
		depth_desc.type = VK_IMAGE_TYPE_2D;
		depth_desc.format = VK_FORMAT_D32_SFLOAT;
		depth_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		depth_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depth_desc.create_flags = 0u;
		auto depth = forge_image_new(forge, depth_desc);

		if (pass)
		{
			auto old_color = pass->description.colors[0].image;
			auto old_depth = pass->description.depth.image;

			forge_image_destroy(forge, old_color);
			forge_image_destroy(forge, old_depth);

			auto desc = pass->description;
			desc.colors[0].image = color;
			desc.depth.image = depth;
			forge_render_pass_update(forge, desc, pass);
		}
		else
		{
			ForgeAttachmentDescription color_attachment_desc{};
			color_attachment_desc.image = color;
			color_attachment_desc.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR; // TODO: This should be provided by the user
			color_attachment_desc.store_op = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_desc.clear_action.color[0] = 1.0f;
			color_attachment_desc.clear_action.color[1] = 0.0f;
			color_attachment_desc.clear_action.color[2] = 1.0f;
			color_attachment_desc.clear_action.color[3] = 1.0f;

			ForgeAttachmentDescription depth_attachment_desc{};
			depth_attachment_desc.image = depth;
			depth_attachment_desc.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depth_attachment_desc.store_op = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachment_desc.clear_action.depth = 1.0f;

			ForgeRenderPassDescription pass_desc{};
			pass_desc.colors[0] = color_attachment_desc;
			pass_desc.depth = depth_attachment_desc;
			frame->pass = forge_render_pass_new(forge, pass_desc);
		}
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

		forge_render_pass_destroy(forge, frame->pass);
	}

	static void
	_forge_pass_swapchain_blit(Forge* forge, ForgeFrame* frame)
	{
		VkResult res;

		auto swapchain = frame->swapchain;
		auto command_buffer = frame->command_buffer;
		auto image_available = swapchain->image_available[swapchain->frame_index % FORGE_SWAPCHIAN_INFLIGH_FRAMES];
		auto fence = swapchain->fence[swapchain->frame_index % FORGE_SWAPCHIAN_INFLIGH_FRAMES];
		auto extent = swapchain->description.extent;

		res = vkWaitForFences(forge->device, 1u, &fence, VK_TRUE, UINT64_MAX);
		VK_RES_CHECK(res);

		res = vkResetFences(forge->device, 1u, &fence);
		VK_RES_CHECK(res);

		res = vkAcquireNextImageKHR(forge->device, swapchain->handle, UINT64_MAX, image_available, VK_NULL_HANDLE, &swapchain->image_index);
		VK_RES_CHECK(res);

		auto src_image = frame->pass->description.colors[0].image;

		ForgeImage dst_image {};
		dst_image.handle = swapchain->images[swapchain->image_index];
		dst_image.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		dst_image.layout = VK_IMAGE_LAYOUT_UNDEFINED;

		forge_image_layout_transition(forge, command_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_image);
		forge_image_layout_transition(forge, command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &dst_image);

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
		vkCmdBlitImage(command_buffer, src_image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &image_blit, VK_FILTER_NEAREST);

		forge_image_layout_transition(forge, command_buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &dst_image);
	}

	ForgeFrame*
	forge_frame_new(Forge* forge)
	{
		auto frame = new ForgeFrame();
		forge->offscreen_frames[forge->offscreen_frames_count++] = frame;

		return frame;
	}

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeSwapchainDescription swapchain_desc)
	{
		auto frame = new ForgeFrame();
		forge->swapchain_frame = frame;

		frame->swapchain = forge_swapchain_new(forge, swapchain_desc);
		if (frame->swapchain == nullptr)
		{
			forge_frame_destroy(forge, frame);
			return nullptr;
		}

		return frame;
	}

	void
	forge_frame_prepare(Forge* forge, ForgeFrame* frame, ForgeShader* shader, ForgeBindingList* binding_list, uint32_t width, uint32_t height)
	{
		frame->command_buffer = forge_command_buffer_acquire(forge, forge->command_buffer_manager, true);
		frame->set = forge_descriptor_set_acquire(forge, forge->descriptor_set_manager, shader, binding_list);

		for (uint32_t i = 0; i < FORGE_MAX_IMAGE_BINDINGS; ++i)
		{
			auto image = binding_list->images[i];
			if (image == nullptr)
				continue;

			forge_image_layout_transition(forge, frame->command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image);
		}

		if (frame->swapchain)
		{
			forge_swapchain_update(forge, frame->swapchain);
		}

		_forge_frame_pass_update(forge, frame, width, height);

		if (frame->pass->handle != shader->active_pass)
		{
			if (shader->pipeline != VK_NULL_HANDLE)
			{
				forge_deletion_queue_push(forge, forge->deletion_queue, shader->pipeline);
			}

			_forge_shader_pipeline_init(forge, shader, frame->pass->handle);
		}
	}

	bool
	forge_frame_begin(Forge* forge, ForgeFrame* frame)
	{
		forge_render_pass_begin(forge, frame->command_buffer, frame->pass);

		return true;
	}

	void
	forge_frame_draw(Forge* forge, ForgeFrame* frame, uint32_t vertex_count)
	{
		auto command_buffer = frame->command_buffer;

		vkCmdDraw(command_buffer, vertex_count, 1u, 0u, 0u);
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

		forge_command_buffer_release(forge, forge->command_buffer_manager, frame->command_buffer);
	}

	ForgeImage*
	forge_frame_color_attachment(Forge* forge, ForgeFrame* frame)
	{
		auto pass = frame->pass;
		assert(pass);

		return pass->description.colors[0].image;
	}

	ForgeImage*
	forge_frame_depth_attachment(Forge* forge, ForgeFrame* frame)
	{
		auto pass = frame->pass;
		assert(pass);

		return pass->description.depth.image;
	}

	void
	forge_frame_bind_resources(Forge* forge, ForgeFrame* frame, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		auto command_buffer = frame->command_buffer;
		auto set = frame->set;

		for (uint32_t i = 0; i < FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS; ++i)
		{
			auto uniform = binding_list->uniforms[i];
			if (uniform.first == 0)
				continue;

			shader->uniform_offsets[i] = forge_dynamic_memory_write(forge, forge->uniform_memory, uniform.first, forge->physical_device_limits.minUniformBufferOffsetAlignment, uniform.second);
		}

		VkDeviceSize offset{};

		for (auto& vertex_buffer : binding_list->vertex_buffers)
		{
			if (vertex_buffer)
			{
				vkCmdBindVertexBuffers(command_buffer, 0u, 1u, &vertex_buffer->handle, &offset);
			}
		}

		if (binding_list->index_buffer)
		{
			vkCmdBindIndexBuffer(command_buffer, binding_list->index_buffer->handle, 0u, VK_INDEX_TYPE_UINT32);
		}

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline_layout, 0u, 1u, &set, shader->uniforms_count, shader->uniform_offsets);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline);
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