#pragma once

#include "ForgeSwapchain.h"
#include "ForgeShader.h"
#include "ForgeImage.h"

#include <vulkan/vulkan.h>

#include <functional>

namespace forge
{
	struct Forge;
	struct ForgeRenderPass;
	struct ForgeSwapchain;
	struct ForgeBindingList;

	static constexpr uint32_t FORGE_FRAME_MAX_UNIFORM_MEMORY = 16 << 20;
	static constexpr uint32_t FORGE_FRAME_MAX_VERTEX_BUFFERS = 16u;
	static constexpr uint32_t FORGE_FRAME_MAX_IMAGES = 16u;

	struct ForgeFrameResourcesList
	{
		std::pair<uint32_t, void*> uniforms[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
		ForgeBuffer* vertex_buffers[FORGE_FRAME_MAX_VERTEX_BUFFERS];
		ForgeBuffer* index_buffer;
		ForgeImage* images[FORGE_FRAME_MAX_IMAGES];
		ForgeShader* shader;
	};

	struct ForgeFrame
	{
		ForgeSwapchain* swapchain;
		ForgeRenderPass* pass;
		ForgeFrameResourcesList resources_list;
		VkCommandBuffer command_buffer;
		VkDescriptorSet set;
	};

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeImageDescription color, ForgeImageDescription depth);

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeSwapchainDescription swapchain_desc);

	void
	forge_frame_prepare(Forge* forge, ForgeFrame* frame, ForgeShader* shader, ForgeBindingList* binding_list, uint32_t width, uint32_t height);

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