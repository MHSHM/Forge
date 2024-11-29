#pragma once

#include "ForgeRenderPass.h"

#include <vulkan/vulkan.h>

#include <string>

#include <shaderc/shaderc.hpp>

namespace forge
{
	struct Forge;

	static constexpr uint32_t FORGE_SHADER_MAX_INPUT_ATTRIBUTES = 16u;
	static constexpr uint32_t FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS = 8u;

	enum FORGE_SHADER_STAGE
	{
		FORGE_SHADER_STAGE_VERTEX,
		FORGE_SHADER_STAGE_FRAGMENT,
		FORGE_SHADER_STAGE_COUNT,
	};

	struct ForgeInputAttributeDescription
	{
		std::string name;
		uint32_t location;
		VkFormat format;
		uint32_t offset;
	};

	struct ForgeUniformBlockDescription
	{
		std::string name;
		uint32_t size;
		VkShaderStageFlags stages;
	};

	struct ForgePipelineDescription
	{
		VkVertexInputBindingDescription bindings[FORGE_SHADER_MAX_INPUT_ATTRIBUTES];
		VkPrimitiveTopology topology;
		VkPolygonMode polygon_mode;
		VkCullModeFlags cull_mode;
		VkFrontFace front_face;
		VkBool32 depth_test;
		VkBool32 depth_write;
		VkCompareOp depth_compare_op;
		VkPipelineColorBlendAttachmentState blend_desc[FORGE_RENDER_PASS_MAX_ATTACHMENTS];
		VkRenderPass pass;
	};

	struct ForgeShaderDescription
	{
		std::string name;
		ForgeInputAttributeDescription attributes[FORGE_SHADER_MAX_INPUT_ATTRIBUTES];
		ForgeUniformBlockDescription uniforms[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
	};

	struct ForgeShader
	{
		VkPipeline pipeline;
		VkPipelineLayout pipeline_layout;
		VkDescriptorSetLayout descriptor_set_layout;
		VkShaderModule modules[FORGE_SHADER_STAGE_COUNT];
		shaderc::SpvCompilationResult spirv[FORGE_SHADER_STAGE_COUNT];
		uint64_t uniform_offsets[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
		ForgeShaderDescription description;
		ForgePipelineDescription pipeline_description;
	};

	ForgeShader*
	forge_shader_new(Forge* forge, ForgePipelineDescription pipeline_description, const char* name, const char* shader_source_code);

	void
	forge_shader_destroy(Forge* forge, ForgeShader* shader);
};