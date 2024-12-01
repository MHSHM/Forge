#pragma once

#include "ForgeRenderPass.h"

#include <vulkan/vulkan.h>

#include <string>
#include <array>

#include <shaderc/shaderc.hpp>

namespace forge
{
	struct Forge;
	struct ForgeRenderPass;

	static constexpr uint32_t FORGE_SHADER_MAX_INPUT_ATTRIBUTES = 16u;
	static constexpr uint32_t FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS = 8u;
	static constexpr uint32_t FORGE_SHADER_MAX_IMAGES = 16u;

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

	struct ForgeShaderImageDescription
	{
		std::string name;
		VkShaderStageFlags stages;
		VkFormat format;
		VkImageViewType type;
		bool storage;
	};

	struct ForgePipelineDescription
	{
		VkVertexInputBindingDescription bindings[FORGE_SHADER_MAX_INPUT_ATTRIBUTES];
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
		VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkBool32 depth_test = VK_TRUE;
		VkBool32 depth_write = VK_TRUE;
		VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;
		VkPipelineColorBlendAttachmentState blend_desc[FORGE_RENDER_PASS_MAX_ATTACHMENTS];
		ForgeRenderPass* pass; // Doesn't own it
	};

	struct ForgeShaderDescription
	{
		std::string name;
		ForgeInputAttributeDescription attributes[FORGE_SHADER_MAX_INPUT_ATTRIBUTES];
		ForgeUniformBlockDescription uniforms[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
		ForgeShaderImageDescription images[FORGE_SHADER_MAX_IMAGES];
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