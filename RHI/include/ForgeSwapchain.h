#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace forge
{ 
	struct Forge;

	struct ForgeSwapchainDescription
	{
		void* window;
		VkPresentModeKHR present_mode;
		VkExtent2D extent;
		VkFormat format;
		uint32_t images_count;
	};

	struct ForgeSwapchain
	{
		VkSwapchainKHR handle;
		VkSurfaceKHR surface;
		VkColorSpaceKHR color_space;
		std::vector<VkImage> images;
		ForgeSwapchainDescription description;
	};

	ForgeSwapchain*
	forge_swapchain_new(Forge* forge, ForgeSwapchainDescription description);

	bool
	forge_swapchain_update(Forge* forge, ForgeSwapchain* swapchain);

	void
	forge_swapchain_destroy(Forge* forge, ForgeSwapchain* swapchain);
};