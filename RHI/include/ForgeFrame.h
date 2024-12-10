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
	struct ForgeBindingList;
	struct ForgeShader;

	static constexpr uint32_t FORGE_FRAME_MAX_UNIFORM_MEMORY = 16 << 20;

	struct ForgeFrame
	{
		ForgeSwapchain* swapchain;
		ForgeRenderPass* pass;
		VkCommandBuffer command_buffer;
		VkDescriptorSet set;
		uint32_t current_frame;
		bool pass_updated;
	};

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeImage* color, ForgeImage* depth);

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeSwapchainDescription swapchain_desc);

	void
	forge_frame_prepare(Forge* forge, ForgeFrame* frame, ForgeShader* shader, ForgeBindingList* binding_list);

	bool
	forge_frame_begin(Forge* forge, ForgeFrame* frame);

	void
	forge_frame_draw(Forge* forge, ForgeFrame* frame, uint32_t vertex_count);

	void
	forge_frame_end(Forge* forge, ForgeFrame* frame);

	void
	forge_frame_bind_resources(Forge* forge, ForgeFrame* frame, ForgeShader* shader, ForgeBindingList* binding_list);

	void
	forge_frame_destroy(Forge* forge, ForgeFrame* frame);
};