#include "Forge.h"
#include "ForgeRenderPass.h"
#include "ForgeImage.h"

namespace forge
{
	static bool
	_forge_render_pass_init(Forge* forge, ForgeRenderPass* render_pass)
	{
		auto& render_pass_desc = render_pass->description;

		VkAttachmentDescription attachments[FORGE_RENDER_PASS_MAX_ATTACHMENTS + 1] = {};
		uint32_t attachments_count = 0;

		for (uint32_t i = 0; i < FORGE_RENDER_PASS_MAX_ATTACHMENTS; ++i)
		{
			auto& color_attachment_desc = render_pass_desc.colors[i];
			if (color_attachment_desc.image == nullptr)
				continue;

			auto& color_attachment = attachments[attachments_count];
			auto& image_desc = color_attachment_desc.image->description;

			color_attachment.format = image_desc.format;
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			color_attachment.loadOp = color_attachment_desc.load_op;
			color_attachment.storeOp = color_attachment_desc.store_op;
			color_attachment.initialLayout = color_attachment_desc.initial_layout;
			color_attachment.finalLayout = color_attachment_desc.final_layout;

			attachments_count++;
		}

		if (render_pass_desc.depth.image != nullptr)
		{
			auto& depth_attachment = attachments[attachments_count];
			auto& depth_attachment_desc = render_pass_desc.depth;
			auto& image_desc = depth_attachment_desc.image->description;

			depth_attachment.format = image_desc.format;
			depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depth_attachment.loadOp = depth_attachment_desc.load_op;
			depth_attachment.storeOp = depth_attachment_desc.store_op;
			depth_attachment.initialLayout = depth_attachment_desc.initial_layout;
			depth_attachment.finalLayout = depth_attachment_desc.final_layout;

			attachments_count++;
		}

		return true;
	}

	static void
	_forge_render_pass_free(Forge* forge, ForgeRenderPass* render_pass)
	{
		if (render_pass->framebuffer)
		{
			vkDestroyFramebuffer(forge->device, render_pass->framebuffer, nullptr);
		}

		if (render_pass->handle)
		{
			vkDestroyRenderPass(forge->device, render_pass->handle, nullptr);
		}
	}

	ForgeRenderPass*
	forge_render_pass_new(Forge* forge, ForgeRenderPassDescription description)
	{
		auto render_pass = new ForgeRenderPass();
		render_pass->description = description;

		if (_forge_render_pass_init(forge, render_pass) == false)
		{
			forge_render_pass_destroy(forge, render_pass);
			return nullptr;
		}

		return render_pass;
	}

	void
	forge_render_pass_destroy(Forge* forge, ForgeRenderPass* render_pass)
	{
		if (render_pass)
		{
			_forge_render_pass_free(forge, render_pass);
			delete render_pass;
		}
	}
};