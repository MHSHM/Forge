#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace forge
{
	struct Forge;

	struct ForgeImageDescription
	{
		std::string name;
		VkExtent3D extent;
		VkImageType type;
		VkFormat format;
		VkImageUsageFlags usage;
		VkMemoryPropertyFlags memory_properties;
		VkImageCreateFlags create_flags;
		bool mipmaps;
	};

	struct ForgeImage
	{
		VkImage handle;
		VkImageView shader_view;
		VkImageView render_target_view;
		VkSampler sampler;
		ForgeImageDescription description;
	};

	ForgeImage*
	forge_image_new(Forge* forge, ForgeImageDescription description);

	void
	forge_image_write(Forge* forge, ForgeImage* image, uint32_t layer, uint32_t size, void* data);

	void
	forge_image_mipmaps_generate(Forge* forge, VkCommandBuffer command_buffer, ForgeImage* image);

	void
	forge_image_destroy(Forge* forge, ForgeImage* image);
};