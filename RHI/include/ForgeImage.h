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
		VkFilter mag_filter = VK_FILTER_LINEAR;
		VkFilter min_filter = VK_FILTER_LINEAR;
		VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerAddressMode address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkSamplerAddressMode address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		bool mipmaps = false;
	};

	struct ForgeImage
	{
		VkImage handle;
		VkDeviceMemory memory;
		VkImageView shader_view;
		VkImageView render_target_view;
		VkImageViewType view_type;
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