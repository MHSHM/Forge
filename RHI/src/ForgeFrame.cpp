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
		ForgeAttachmentDescription color_attachment_desc {};
		color_attachment_desc.image = color;
		color_attachment_desc.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR; // TODO: This should be provided by the user
		color_attachment_desc.store_op = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_desc.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_desc.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		color_attachment_desc.clear_action.color[0] = 0.0f;
		color_attachment_desc.clear_action.color[1] = 0.0f;
		color_attachment_desc.clear_action.color[2] = 0.0f;
		color_attachment_desc.clear_action.color[3] = 0.0f;

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
		auto res = vkCreateSemaphore(forge->device, &rendering_done_info, NULL, &frame->rendering_done);
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

		frame->frame_index = 0u;

		return true;
	}

	static bool
	_forge_frame_init(Forge* forge, ForgeImage* color, ForgeImage* depth, ForgeFrame* frame)
	{
		if (_forge_frame_pass_init(forge, frame, color, depth) == false)
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
	_forge_frame_init(Forge* forge, ForgeSwapchainDescription swapchain_desc, ForgeFrame* frame)
	{
		frame->swapchain = forge_swapchain_new(forge, swapchain_desc);

		if (frame->swapchain == nullptr)
		{
			log_error("Failed to initialize frame's swapchain");
			return false;
		}

		ForgeImageDescription color_desc {};
		color_desc.name = "Frame Color";
		color_desc.extent = {swapchain_desc.extent.width, swapchain_desc.extent.height, 1};
		color_desc.type = VK_IMAGE_TYPE_2D;
		color_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
		color_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		color_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		color_desc.create_flags = 0u;
		auto color = forge_image_new(forge, color_desc);

		ForgeImageDescription depth_desc {};
		depth_desc.name = "Frame Depth";
		depth_desc.extent = { swapchain_desc.extent.width, swapchain_desc.extent.height, 1 };
		depth_desc.type = VK_IMAGE_TYPE_2D;
		depth_desc.format = VK_FORMAT_D32_SFLOAT;
		depth_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depth_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depth_desc.create_flags = 0u;
		auto depth = forge_image_new(forge, depth_desc);

		if (_forge_frame_pass_init(forge, frame, color, depth) == false)
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
		vkDestroySemaphore(forge->device, frame->rendering_done, nullptr);
		forge_dynamic_memory_destroy(forge, frame->uniform_memory);
		forge_descriptor_set_manager_destroy(forge, frame->descriptor_set_manager);
		forge_deferred_queue_destroy(forge, frame->deferred_queue);
		forge_render_pass_destroy(forge, frame->pass);
	}

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeImage* color, ForgeImage* depth)
	{
		auto frame = new ForgeFrame();

		if (_forge_frame_init(forge, color, depth, frame) == false)
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

		if (_forge_frame_init(forge, swapchain_desc, frame) == false)
		{
			forge_frame_destroy(forge, frame);
			return nullptr;
		}

		return frame;
	}

	bool
	forge_frame_begin(Forge* forge, ForgeFrame* frame)
	{
		return true;
	}

	void
	forge_frame_end(Forge* forge, ForgeFrame* frame)
	{
	
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