#include "Forge.h"
#include "ForgeFrame.h"
#include "ForgeSwapchain.h"
#include "ForgeRenderPass.h"
#include "ForgeDeferedQueue.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeDynamicMemory.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"

namespace forge
{
	static bool
	_forge_frame_init(Forge* forge, ForgeImage* color, ForgeImage* depth, ForgeFrame* frame)
	{
		ForgeAttachmentDescription color_attachment_desc {};
		color_attachment_desc.image = color;
		color_attachment_desc.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
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
	_forge_frame_init(Forge* forge, ForgeSwapchainDescription swapchain_desc, ForgeFrame* frame)
	{
		return true;
	}

	static void
	_forge_frame_free(Forge* forge, ForgeFrame* frame)
	{
		if (frame->swapchain)
		{
		
		}
		else
		{
			
		}
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
		return nullptr;
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