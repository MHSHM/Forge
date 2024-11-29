#include "Forge.h"
#include "ForgeShader.h"
#include "ForgeLogger.h"

#include <vector>
#include <assert.h>

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

	static bool
	_forge_shader_pipeline_layout_init(
		Forge* forge,
		VkDescriptorSetLayout set_layout,
		ForgeShaderDescription shader_description,
		ForgePipelineDescription pipeline_description,
		VkPipelineLayout* layout)
	{
		VkResult res;

		std::vector<VkDescriptorSetLayoutBinding> bindings;

		for (uint32_t i = 0; i < FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS; ++i)
		{
			const ForgeUniformBlockDescription& uniform = shader_description.uniforms[i];

			if (uniform.size > 0)
			{
				VkDescriptorSetLayoutBinding binding = {};
				binding.binding = i;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				binding.descriptorCount = 1;
				binding.stageFlags = uniform.stages;
				binding.pImmutableSamplers = nullptr;

				bindings.push_back(binding);
			}
		}

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &set_layout;
		res = vkCreatePipelineLayout(forge->device, &pipeline_layout_info, nullptr, layout);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize the pipeline layout");
			return false;
		}

		return true;
	}

	static bool
	_forge_shader_pipeline_init(Forge* forge, ForgePipelineDescription pipeline_description, VkPipeline* pipeline)
	{
		return true;
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
		}
	}

	static bool
	_forge_shader_description_init(Forge* forge, const char* shader_source_code, ForgeShader* shader)
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

		return true;
	}

	static bool
	_forge_shader_module_init(Forge* forge, FORGE_SHADER_STAGE stage, const char* source, ForgeShader* shader)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.AddMacroDefinition(stage == FORGE_SHADER_STAGE_VERTEX ? "VERTEX_SHADER" : "FRAGMENT_SHADER");
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, (shaderc_shader_kind)stage, source, options);

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

		shader->spirv[stage] = std::move(module);

		return true;
	}

	static bool
	_forge_shader_init(Forge* forge, ForgePipelineDescription pipeline_description, const char* shader_source_code, ForgeShader* shader)
	{
		if (_forge_shader_module_init(forge, FORGE_SHADER_STAGE_VERTEX, shader_source_code, shader) == false)
		{
			return false;
		}

		if (_forge_shader_module_init(forge, FORGE_SHADER_STAGE_FRAGMENT, shader_source_code, shader) == false)
		{
			return false;
		}

		if (_forge_shader_description_init(forge, shader_source_code, shader) == false)
		{
			return false;
		}

		return true;
	}

	static void
	_forge_shader_free(Forge* forge, ForgeShader* shader)
	{
		
	}

	ForgeShader*
	forge_shader_new(Forge* forge, ForgePipelineDescription pipeline_description, const char* shader_source_code)
	{
		ForgeShader* shader = new ForgeShader();
		shader->pipeline_description = pipeline_description;

		if (_forge_shader_init(forge, pipeline_description, shader_source_code, shader) == false)
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