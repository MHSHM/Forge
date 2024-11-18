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

	struct ForgeImageData
	{
		// In case of cubemap each entry will hold the data of each face
		// otherwise the data will be in data[0]
		void* data[6];
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
	forge_image_write(Forge* forge, ForgeImage* image, ForgeImageData data);

	void
	forge_image_destroy(Forge* forge, ForgeImage* image);
};