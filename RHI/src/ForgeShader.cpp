#include "Forge.h"
#include "ForgeShader.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeBindingList.h"
#include "ForgeDeletionQueue.h"

#include <vector>
#include <assert.h>

#include <spirv-headers/spirv.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_common.hpp>

using namespace spirv_cross;

namespace forge
{
	static VkFormat
	_forge_spirv_type_vk_format(SPIRType type)
	{
		if (type.basetype == SPIRType::BaseType::Float)
		{
			if (type.vecsize == 1)
			{
				return VK_FORMAT_R32_SFLOAT;
			}
			else if (type.vecsize == 2)
			{
				return VK_FORMAT_R32G32_SFLOAT;
			}
			else if (type.vecsize == 3)
			{
				return VK_FORMAT_R32G32B32_SFLOAT;
			}
			else if (type.vecsize == 4)
			{
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			else
			{
				assert(false); // unreachable
			}
		}

		return VK_FORMAT_R8G8B8A8_UNORM;
	}

	static VkShaderStageFlagBits
	_forge_shader_stage_vk_stage(FORGE_SHADER_STAGE stage)
	{
		switch (stage)
		{
		case FORGE_SHADER_STAGE_VERTEX:	return VK_SHADER_STAGE_VERTEX_BIT;
		case FORGE_SHADER_STAGE_FRAGMENT:	return VK_SHADER_STAGE_FRAGMENT_BIT;
		default:
			assert(false);
			break;
		}

		return (VkShaderStageFlagBits)0u;
	}

	bool
	_forge_shader_pipeline_init(Forge* forge, ForgeShader* shader, VkRenderPass pass)
	{
		if (pass == shader->pipeline_description.pass)
		{
			return true;
		}

		VkResult res;

		const auto& pipeline_description = shader->pipeline_description;
		const auto& shader_description = shader->description;

		uint32_t shader_stage_count = 0;
		VkPipelineShaderStageCreateInfo shader_stage_create_infos[FORGE_SHADER_STAGE_COUNT]{};
		for (uint32_t i = 0; i < FORGE_SHADER_STAGE_COUNT; ++i)
		{
			if (shader->modules[i] == VK_NULL_HANDLE)
				continue;

			auto& shader_stage_info = shader_stage_create_infos[shader_stage_count];
			shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage_info.stage = _forge_shader_stage_vk_stage(static_cast<FORGE_SHADER_STAGE>(i));
			shader_stage_info.module = shader->modules[i];
			shader_stage_info.pName = "main";

			++shader_stage_count;
		}

		uint32_t attributes_count = 0;
		VkVertexInputAttributeDescription attributes[FORGE_SHADER_MAX_INPUT_ATTRIBUTES] {};
		for (uint32_t i = 0; i < FORGE_SHADER_MAX_INPUT_ATTRIBUTES; ++i)
		{
			auto& attribute_description = shader_description.attributes[i];
			if (attribute_description.name.empty() == true)
				continue;

			auto& attribute = attributes[attributes_count];
			attribute.binding = i;
			attribute.format = attribute_description.format;
			attribute.location = attribute_description.location;
			attribute.offset = attribute_description.offset;

			++attributes_count;
		}

		uint32_t bindings_count = 0u;
		VkVertexInputBindingDescription bindings[FORGE_SHADER_MAX_INPUT_ATTRIBUTES];
		for (uint32_t i = 0; i < FORGE_SHADER_MAX_INPUT_ATTRIBUTES; ++i)
		{
			auto& binding = pipeline_description.bindings[i];
			if (binding.stride == 0u)
				continue;

			auto& _binding = bindings[bindings_count];
			_binding.binding = binding.binding;
			_binding.inputRate = binding.inputRate;
			_binding.stride = binding.stride;

			++bindings_count;
		}

		VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
		vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state.vertexBindingDescriptionCount = bindings_count;
		vertex_input_state.pVertexBindingDescriptions = bindings;
		vertex_input_state.vertexAttributeDescriptionCount = attributes_count;
		vertex_input_state.pVertexAttributeDescriptions = attributes;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
		input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_state.topology = pipeline_description.topology;

		VkPipelineRasterizationStateCreateInfo rasterization_state = {};
		rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_state.polygonMode = pipeline_description.polygon_mode;
		rasterization_state.cullMode = pipeline_description.cull_mode;
		rasterization_state.frontFace = pipeline_description.front_face;
		rasterization_state.depthClampEnable = VK_FALSE;
		rasterization_state.rasterizerDiscardEnable = VK_FALSE;
		rasterization_state.depthBiasEnable = VK_FALSE;
		rasterization_state.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisample_state = {};
		multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
		depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_state.depthTestEnable = pipeline_description.depth_test;
		depth_stencil_state.depthWriteEnable = pipeline_description.depth_write;
		depth_stencil_state.depthCompareOp = pipeline_description.depth_compare_op;
		depth_stencil_state.stencilTestEnable = VK_FALSE;

		uint32_t blend_attachemnts_count = 0u;
		VkPipelineColorBlendAttachmentState blend_attachments[FORGE_RENDER_PASS_MAX_ATTACHMENTS]{};
		for (uint32_t i = 0; i < FORGE_RENDER_PASS_MAX_ATTACHMENTS; ++i)
		{
			auto& attachment = pipeline_description.blend_desc[i];
			if (attachment.colorWriteMask == 0u)
				continue;

			auto& _attachment = blend_attachments[blend_attachemnts_count];
			_attachment.blendEnable = attachment.blendEnable;
			_attachment.srcColorBlendFactor = attachment.srcColorBlendFactor;
			_attachment.dstColorBlendFactor = attachment.dstColorBlendFactor;
			_attachment.colorBlendOp = attachment.colorBlendOp;
			_attachment.srcAlphaBlendFactor = attachment.srcAlphaBlendFactor;
			_attachment.dstAlphaBlendFactor = attachment.dstAlphaBlendFactor;
			_attachment.alphaBlendOp = attachment.alphaBlendOp;
			_attachment.colorWriteMask = attachment.colorWriteMask;

			++blend_attachemnts_count;
		}

		VkPipelineColorBlendStateCreateInfo color_blend_state = {};
		color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_state.logicOpEnable = VK_FALSE;
		color_blend_state.attachmentCount = blend_attachemnts_count;
		color_blend_state.pAttachments = blend_attachments;

		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		VkDynamicState dynamic_states[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		dynamic_state.pDynamicStates = dynamic_states;
		dynamic_state.dynamicStateCount = 2u;

		VkPipelineViewportStateCreateInfo viewport_state {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.scissorCount = 1;

		// Create the graphics pipeline
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = shader_stage_count;
		pipeline_create_info.pStages = shader_stage_create_infos;
		pipeline_create_info.pVertexInputState = &vertex_input_state;
		pipeline_create_info.pInputAssemblyState = &input_assembly_state;
		pipeline_create_info.pRasterizationState = &rasterization_state;
		pipeline_create_info.pMultisampleState = &multisample_state;
		pipeline_create_info.pDepthStencilState = &depth_stencil_state;
		pipeline_create_info.pColorBlendState = &color_blend_state;
		pipeline_create_info.pDynamicState = &dynamic_state;
		pipeline_create_info.pViewportState = &viewport_state;
		pipeline_create_info.layout = shader->pipeline_layout;
		pipeline_create_info.renderPass = pass;
		pipeline_create_info.subpass = 0;
		res = vkCreateGraphicsPipelines(forge->device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &shader->pipeline);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS) {
			log_error("Failed to initialize graphics pipeline");
			return false;
		}

		_forge_debug_obj_name_set(forge, (uint64_t)shader->pipeline, VK_OBJECT_TYPE_PIPELINE, shader->description.name.c_str());

		shader->pipeline_description.pass = pass;

		return true;
	}

	static bool
	_forge_shader_pipeline_layout_init(Forge* forge, ForgeShader* shader)
	{
		VkResult res;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &shader->descriptor_set_layout;
		res = vkCreatePipelineLayout(forge->device, &pipeline_layout_info, nullptr, &shader->pipeline_layout);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize the pipeline layout");
			return false;
		}

		_forge_debug_obj_name_set(forge, (uint64_t)shader->pipeline_layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, shader->description.name.c_str());

		return true;
	}

	static bool
	_forge_shader_descriptor_set_layout_init(Forge* forge, ForgeShader* shader)
	{
		uint32_t count = 0;
		VkDescriptorSetLayoutBinding bindings[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS + FORGE_MAX_IMAGE_BINDINGS] = {};

		for (uint32_t i = 0; i < FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS; ++i)
		{
			auto uniforms = shader->description.uniforms;
			if (uniforms[i].name.empty() == true)
				continue;

			bindings[count].binding = i;
			bindings[count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			bindings[count].descriptorCount = 1u;
			bindings[count].stageFlags = uniforms[i].stages;

			++count;
		}

		for (uint32_t i = 0; i < FORGE_MAX_IMAGE_BINDINGS; ++i)
		{
			auto images = shader->description.images;
			if (images[i].name.empty() == true)
				continue;

			bindings[count].binding = i;
			bindings[count].descriptorType = images[i].storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[count].descriptorCount = 1u;
			bindings[count].stageFlags = images[i].stages;

			++count;
		}

		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = count;
		info.pBindings = bindings;
		auto res = vkCreateDescriptorSetLayout(forge->device, &info, nullptr, &shader->descriptor_set_layout);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize the descriptor set layout");
			return false;
		}

		_forge_debug_obj_name_set(forge, (uint64_t)shader->descriptor_set_layout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, shader->description.name.c_str());

		return true;
	}

	static void
	_forge_shader_images_init(Forge* forge, FORGE_SHADER_STAGE stage, ForgeShader* shader)
	{
		auto& module = shader->spirv[stage];
		auto& shader_description = shader->description;
		Compiler compiler({module.cbegin(), module.end()});

		auto resources = compiler.get_shader_resources();
		for (auto& spv_image : resources.sampled_images)
		{
			auto binding = compiler.get_decoration(spv_image.id, spv::DecorationBinding);
			assert(binding < FORGE_MAX_IMAGE_BINDINGS);

			auto& image = shader_description.images[binding];
			if (image.name.empty() == false)
			{
				image.stages |= _forge_shader_stage_vk_stage(stage);
				continue;
			}

			auto type = compiler.get_type(spv_image.type_id);
			image.name = spv_image.name;
			image.stages = _forge_shader_stage_vk_stage(stage);
			image.storage = false;
		}

		for (auto& spv_image : resources.storage_images)
		{
			auto binding = compiler.get_decoration(spv_image.id, spv::DecorationBinding);
			assert(binding < FORGE_MAX_IMAGE_BINDINGS);

			auto& image = shader_description.images[binding];
			if (image.name.empty() == false)
			{
				image.stages |= _forge_shader_stage_vk_stage(stage);
				continue;
			}

			auto type = compiler.get_type(spv_image.type_id);
			image.name = spv_image.name;
			image.stages = _forge_shader_stage_vk_stage(stage);
			image.storage = true;
		}
	}

	static void
	_forge_shader_uniform_blocks_init(Forge* forge, FORGE_SHADER_STAGE stage, ForgeShader* shader)
	{
		auto& module = shader->spirv[stage];
		auto& shader_description = shader->description;
		Compiler compiler({module.cbegin(), module.end()});

		auto resources = compiler.get_shader_resources();
		for (auto& spv_uniform : resources.uniform_buffers)
		{
			auto binding = compiler.get_decoration(spv_uniform.id, spv::DecorationBinding);
			assert(binding < FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS);

			auto& uniform = shader_description.uniforms[binding];
			if (uniform.name.empty() == false)
			{
				uniform.stages |= _forge_shader_stage_vk_stage(stage);
				continue;
			}

			uniform.name = spv_uniform.name;
			uniform.size = (uint32_t)compiler.get_declared_struct_size(compiler.get_type(spv_uniform.type_id));
			uniform.stages = _forge_shader_stage_vk_stage(stage);

			shader->uniforms_count++;
		}
	}

	static bool
	_forge_shader_description_init(Forge* forge, const char* name, const char* shader_source_code, ForgeShader* shader)
	{
		auto& module = shader->spirv[FORGE_SHADER_STAGE_VERTEX];
		auto& shader_description = shader->description;
		Compiler compiler({module.cbegin(), module.end()});

		auto resources = compiler.get_shader_resources();
		for (auto& input : resources.stage_inputs)
		{
			auto binding  = compiler.get_decoration(input.id, spv::DecorationBinding);
			assert(binding < FORGE_SHADER_MAX_INPUT_ATTRIBUTES);

			auto& attribute = shader_description.attributes[binding];
			attribute.name = input.name;
			attribute.location = compiler.get_decoration(input.id, spv::DecorationLocation);
			attribute.format = _forge_spirv_type_vk_format(compiler.get_type(input.type_id));
			attribute.offset = 0u; // ??
		}

		_forge_shader_uniform_blocks_init(forge, FORGE_SHADER_STAGE_VERTEX, shader);
		_forge_shader_uniform_blocks_init(forge, FORGE_SHADER_STAGE_FRAGMENT, shader);

		_forge_shader_images_init(forge, FORGE_SHADER_STAGE_VERTEX, shader);
		_forge_shader_images_init(forge, FORGE_SHADER_STAGE_FRAGMENT, shader);

		shader_description.name = name;

		return true;
	}

	static bool
	_forge_shader_module_init(Forge* forge, FORGE_SHADER_STAGE stage, const char* name, const char* source, ForgeShader* shader)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.AddMacroDefinition(stage == FORGE_SHADER_STAGE_VERTEX ? "VERTEX_SHADER" : "FRAGMENT_SHADER");
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, (shaderc_shader_kind)stage, name, options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			log_error("{}", module.GetErrorMessage().c_str());
			return false;
		}

		VkShaderModuleCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = (uintptr_t)module.cend() - (uintptr_t)module.cbegin();
		info.pCode = module.cbegin();
		auto res = vkCreateShaderModule(forge->device, &info, nullptr, &shader->modules[stage]);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create shader module");
			return false;
		}

		_forge_debug_obj_name_set(forge, (uint64_t)shader->modules[stage], VK_OBJECT_TYPE_SHADER_MODULE, name);

		shader->spirv[stage] = std::move(module);

		return true;
	}

	static bool
	_forge_shader_init(Forge* forge, ForgePipelineDescription pipeline_description, const char* name, const char* shader_source_code, ForgeShader* shader)
	{
		if (_forge_shader_module_init(forge, FORGE_SHADER_STAGE_VERTEX, name, shader_source_code, shader) == false)
		{
			return false;
		}

		if (_forge_shader_module_init(forge, FORGE_SHADER_STAGE_FRAGMENT, name, shader_source_code, shader) == false)
		{
			return false;
		}

		if (_forge_shader_description_init(forge, name, shader_source_code, shader) == false)
		{
			return false;
		}

		if (_forge_shader_descriptor_set_layout_init(forge, shader) == false)
		{
			return false;
		}

		if (_forge_shader_pipeline_layout_init(forge, shader) == false)
		{
			return false;
		}

		return true;
	}

	static void
	_forge_shader_free(Forge* forge, ForgeShader* shader)
	{
		if (shader->modules[FORGE_SHADER_STAGE_FRAGMENT])
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, shader->modules[FORGE_SHADER_STAGE_FRAGMENT]);
		}

		if (shader->modules[FORGE_SHADER_STAGE_VERTEX])
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, shader->modules[FORGE_SHADER_STAGE_VERTEX]);
		}

		if (shader->descriptor_set_layout)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, shader->descriptor_set_layout);
		}

		if (shader->pipeline_layout)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, shader->pipeline_layout);
		}

		if (shader->pipeline)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, shader->pipeline);
		}
	}

	ForgeShader*
	forge_shader_new(Forge* forge, ForgePipelineDescription pipeline_description, const char* name, const char* shader_source_code)
	{
		ForgeShader* shader = new ForgeShader();
		shader->pipeline_description = pipeline_description;

		if (_forge_shader_init(forge, pipeline_description, name, shader_source_code, shader) == false)
		{
			forge_shader_destroy(forge, shader);
			return nullptr;
		}

		return shader;
	}

	void
	forge_shader_destroy(Forge* forge, ForgeShader* shader)
	{
		if (shader)
		{
			_forge_shader_free(forge, shader);
			delete shader;
		}
	}
};