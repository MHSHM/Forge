#pragma once

#include "ForgeSwapchain.h"

#include <vulkan/vulkan.h>

#include <functional>

namespace forge
{
	struct Forge;
	struct ForgeImage;
	struct ForgeRenderPass;
	struct ForgeSwapchain;
	struct ForgeDescriptorSetManager;
	struct ForgeDynamicMemory;

	static constexpr uint32_t FORGE_FRAME_MAX_UNIFORM_MEMORY = 16 << 20;

	struct ForgeFrame
	{
		ForgeSwapchain* swapchain;
		ForgeRenderPass* pass;
		VkCommandPool command_pool;
		VkCommandBuffer command_buffer;
		uint32_t current_frame;
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