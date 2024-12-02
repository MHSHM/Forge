#pragma once

#include "ForgeSwapchain.h"

#include <vulkan/vulkan.h>

namespace forge
{
	struct Forge;
	struct ForgeImage;
	struct ForgeRenderPass;
	struct ForgeSwapchain;
	struct ForgeDescriptorSetManager;
	struct ForgeDynamicMemory;
	struct ForgeDeferredQueue;

	static constexpr uint32_t FORGE_FRAME_MAX_UNIFORM_MEMORY = 16 << 20;

	struct ForgeFrame
	{
		ForgeSwapchain* swapchain;
		ForgeRenderPass* pass;
		ForgeDeferredQueue* deferred_queue;
		ForgeDescriptorSetManager* descriptor_set_manager;
		ForgeDynamicMemory* uniform_memory;
		VkSemaphore rendering_done;
		VkCommandPool command_pool;
		uint32_t frame_index;
	};

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeImage* color, ForgeImage* depth);

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeSwapchainDescription swapchain_desc);

	bool
	forge_frame_begin(Forge* forge, ForgeFrame* frame);

	void
	forge_frame_end(Forge* forge, ForgeFrame* frame);

	void
	forge_frame_destroy(Forge* forge, ForgeFrame* frame);
};