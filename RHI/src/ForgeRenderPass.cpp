#include "Forge.h"
#include "ForgeRenderPass.h"
#include "ForgeImage.h"
#include "ForgeUtils.h"

namespace forge
{
	static bool
	_forge_render_pass_init(Forge* forge, ForgeRenderPass* render_pass)
	{
		auto& render_pass_desc = render_pass->description;

		VkAttachmentReference color_attachments_reference[FORGE_RENDER_PASS_MAX_ATTACHMENTS] = {};
		uint32_t color_attachments_count = 0u;

		VkAttachmentReference depth_attachment_reference = {};

		VkAttachmentDescription attachments[FORGE_RENDER_PASS_MAX_ATTACHMENTS + 1] = {};
		uint32_t attachments_count = 0;

		for (uint32_t i = 0; i < FORGE_RENDER_PASS_MAX_ATTACHMENTS; ++i)
		{
			auto& color_attachment_desc = render_pass_desc.colors[i];
			if (color_attachment_desc.image == nullptr)
				continue;

			auto& color_attachment = attachments[attachments_count];
			auto& color_attachment_reference = color_attachments_reference[attachments_count];
			auto& image_desc = color_attachment_desc.image->description;

			color_attachment.format = image_desc.format;
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			color_attachment.loadOp = color_attachment_desc.load_op;
			color_attachment.storeOp = color_attachment_desc.store_op;
			color_attachment.initialLayout = color_attachment_desc.initial_layout;
			color_attachment.finalLayout = color_attachment_desc.final_layout;

			color_attachment_reference.attachment = i;
			color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachments_count++;
		}
		color_attachments_count = attachments_count;

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

			depth_attachment_reference.attachment = attachments_count;
			depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments_count++;
		}

		VkSubpassDependency subpass_dependency[2] = {};

		subpass_dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency[0].dstSubpass = 0u;
		subpass_dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		subpass_dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		subpass_dependency[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

		subpass_dependency[1].srcSubpass = 0u;
		subpass_dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency[1].srcStageMask = subpass_dependency[0].dstStageMask;
		subpass_dependency[1].dstStageMask = subpass_dependency[0].srcStageMask;
		subpass_dependency[1].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		subpass_dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

		VkSubpassDescription subpass_description {};
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.colorAttachmentCount = color_attachments_count;
		subpass_description.pColorAttachments = color_attachments_reference;
		subpass_description.pDepthStencilAttachment = depth_attachment_reference.layout == VK_IMAGE_LAYOUT_UNDEFINED ? nullptr : &depth_attachment_reference;

		VkRenderPassCreateInfo render_pass_info {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = attachments_count;
		render_pass_info.pAttachments = attachments;
		render_pass_info.subpassCount = 1u;
		render_pass_info.pSubpasses = &subpass_description;
		render_pass_info.dependencyCount = 2u;
		render_pass_info.pDependencies = subpass_dependency;
		auto res = vkCreateRenderPass(forge->device, &render_pass_info, nullptr, &render_pass->handle);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create render pass, the follwoing error code '{}' is reported", _forge_result_to_str(res));
			return false;
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