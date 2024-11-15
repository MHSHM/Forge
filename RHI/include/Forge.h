#pragma once

#include <vulkan/vulkan.h>

namespace forge
{
	struct Forge
	{
		VkInstance instance;
		VkDevice device;

		VkDebugUtilsMessengerEXT debug_messenger;
		PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT;
		PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT;
		PFN_vkCmdBeginDebugUtilsLabelEXT pfn_vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT pfn_vkCmdEndDebugUtilsLabelEXT;
	};

	Forge* forge_new();

	void forge_destroy(Forge* forge);
};