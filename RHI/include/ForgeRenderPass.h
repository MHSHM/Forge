#pragma once

#include <vulkan/vulkan.h>

namespace forge
{
	static constexpr uint32_t FORGE_RENDER_PASS_MAX_ATTACHMENTS = 4u;

	struct Forge;
	struct ForgeImage;

	struct ForgeAttachmentClearAction
	{
		float color[4];
		float depth;
	};

	struct ForgeAttachmentDescription
	{
		ForgeImage* image; // Doesn't own the image
		VkAttachmentLoadOp load_op;
		VkAttachmentStoreOp store_op;
		ForgeAttachmentClearAction clear_action;
	};

	struct ForgeRenderPassDescription
	{
		ForgeAttachmentDescription colors[FORGE_RENDER_PASS_MAX_ATTACHMENTS];
		ForgeAttachmentDescription depth;
	};

	struct ForgeRenderPass
	{
		VkRenderPass handle;
		VkFramebuffer framebuffer;
		uint32_t width;
		uint32_t height;
		ForgeRenderPassDescription description;
	};

	ForgeRenderPass*
	forge_render_pass_new(Forge* forge, ForgeRenderPassDescription description);

	void
	forge_render_pass_begin(Forge* forge, VkCommandBuffer command_buffer, ForgeRenderPass* render_pass);

	void
	forge_render_pass_end(Forge* forge, VkCommandBuffer command_buffer, ForgeRenderPass* render_pass);

	void
	forge_render_pass_update(Forge* forge, ForgeRenderPassDescription description, ForgeRenderPass* render_pass);

	void
	forge_render_pass_destroy(Forge* forge, ForgeRenderPass* render_pass);
};