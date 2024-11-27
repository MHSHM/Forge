#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace forge
{
	struct Forge;

	static constexpr uint32_t FORGE_SHADER_MAX_INPUT_ATTRIBUTES = 16u;
	static constexpr uint32_t FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS = 8u;

	struct ForgeInputAttributeDescription
	{
		std::string name;
		uint32_t location;
		VkFormat format;
		uint32_t binding;
		uint32_t offset;
	};

	struct ForgeUniformBlockDescription
	{
		std::string name;
		uint32_t binding;
		uint32_t size;
	};

	struct ForgePipelineDescription
	{
		// Different vulkan structures for pipeline creation
	};

	struct ForgeShaderDescription
	{
		ForgeInputAttributeDescription attributes[FORGE_SHADER_MAX_INPUT_ATTRIBUTES];
		ForgeUniformBlockDescription uniforms[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
	};

	struct ForgeShader
	{
		VkPipeline pipeline;
		VkPipelineLayout pipeline_layout;
		uint64_t uniform_offsets[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
		ForgeShaderDescription description;
		ForgePipelineDescription pipeline_description
	};

	ForgeShader*
	forge_shader_new(Forge* forge, ForgePipelineDescription pipeline_description, const char* shader_source_code);

	void
	forge_shader_destroy(Forge* forge, ForgeShader* shader);
};