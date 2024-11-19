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
	};

	struct ForgeImage
	{
		VkImage handle;
		VkImageView view;
		VkSampler sampler;
		ForgeImageDescription description;
	};

	ForgeImage*
	forge_image_new(Forge* forge, ForgeImageDescription description);

	void
	forge_image_write(Forge* forge, ForgeImage* image, uint32_t layer, uint32_t size, void* data);

	void
	forge_image_destroy(Forge* forge, ForgeImage* image);
};