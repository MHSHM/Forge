#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace forge
{
	struct Forge
	{
		VkInstance instance;

		VkPhysicalDevice physical_device;
		VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
		VkPhysicalDeviceLimits physical_device_limits;
		std::string physical_device_name;
		uint64_t physical_memory;
		uint32_t queue_family_index;

		VkDevice device;
		VkQueue queue;

		VkDebugUtilsMessengerEXT debug_messenger;
		PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT;
		PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT;
		PFN_vkCmdBeginDebugUtilsLabelEXT pfn_vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT pfn_vkCmdEndDebugUtilsLabelEXT;
	};

	Forge*
	forge_new();

	void
	forge_destroy(Forge* forge);
};