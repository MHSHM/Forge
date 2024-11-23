#pragma once

#include <vulkan/vulkan.h>

namespace forge
{
	static constexpr uint32_t FORGE_RENDER_PASS_MAX_ATTACHMENTS = 4u;

	struct Forge;
	struct ForgeImage;

	struct ForgeAttachmentDescription
	{
		ForgeImage* image; // Doesn't own the image
		VkAttachmentLoadOp load_op;
		VkAttachmentStoreOp store_op;
		VkImageLayout initial_layout;
		VkImageLayout final_layout;
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
		ForgeRenderPassDescription description;
	};

	ForgeRenderPass*
	forge_render_pass_new(Forge* forge, ForgeRenderPassDescription description);

	void
	forge_render_pass_destroy(Forge* forge, ForgeRenderPass* render_pass);
};