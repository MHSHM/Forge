#include "Forge.h"
#include "ForgeRenderPass.h"
#include "ForgeImage.h"
#include "ForgeUtils.h"

namespace forge
{
	static bool
	_forge_render_pass_init(Forge* forge, ForgeRenderPass* render_pass)
	{
		VkResult res;

		auto& render_pass_desc = render_pass->description;

		VkAttachmentReference color_attachments_reference[FORGE_RENDER_PASS_MAX_ATTACHMENTS] = {};
		uint32_t color_attachments_count = 0u;

		VkAttachmentReference depth_attachment_reference = {};

		VkAttachmentDescription attachments[FORGE_RENDER_PASS_MAX_ATTACHMENTS + 1] = {};
		VkImageView attachment_views[FORGE_RENDER_PASS_MAX_ATTACHMENTS + 1] = {};
		uint32_t attachments_count = 0;

		uint32_t width = 0u;
		uint32_t height = 0u;

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

			attachment_views[i] = color_attachment_desc.image->render_target_view;

			width = color_attachment_desc.image->description.extent.width;
			height = color_attachment_desc.image->description.extent.height;

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

			attachment_views[attachments_count] = render_pass_desc.depth.image->render_target_view;

			width = depth_attachment_desc.image->description.extent.width;
			height = depth_attachment_desc.image->description.extent.height;

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
		res = vkCreateRenderPass(forge->device, &render_pass_info, nullptr, &render_pass->handle);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create render pass, the following error code '{}' is reported", _forge_result_to_str(res));
			return false;
		}

		VkFramebufferCreateInfo framebuffer_info {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass->handle;
		framebuffer_info.attachmentCount = attachments_count;
		framebuffer_info.pAttachments = attachment_views;
		framebuffer_info.width = width;
		framebuffer_info.height = height;
		framebuffer_info.layers = 1u;
		res = vkCreateFramebuffer(forge->device, &framebuffer_info, nullptr, &render_pass->framebuffer);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create framebuffer, the follwoing error code '{}', is reported", _forge_result_to_str(res));
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

		bool has_attachment = false;
		int width = -1; int height = -1;
		bool same_dimensions = true;

		for (uint32_t i = 0; i < FORGE_RENDER_PASS_MAX_ATTACHMENTS; ++i)
		{
			if (description.colors[i].image)
			{
				auto extent = description.colors[i].image->description.extent;

				has_attachment = true;

				if (width == -1)
				{
					width  = extent.width;
					height = extent.height;
				}
				else
				{
					if (width == extent.width && height == extent.height)
					{
						same_dimensions = true;
					}
					else
					{
						same_dimensions = false;
					}
				}
			}
		}

		if (description.depth.image)
		{
			auto extent = description.depth.image->description.extent;

			has_attachment = true;

			if (width == -1)
			{
				width  = extent.width;
				height = extent.height;
			}
			else
			{
				if (width == extent.width && height == extent.height)
				{
					same_dimensions = true;
				}
				else
				{
					same_dimensions = false;
				}
			}
		}

		if (has_attachment == false) { log_error("A render pass must have at least one attachment"); return nullptr; }
		if (same_dimensions == false) { log_error("All attachments of a render pass must have the same dimensions"); return nullptr; }

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