#pragma once

#define NOMINMAX
#ifdef _WIN32
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_RES_CHECK(res) assert(res == VK_SUCCESS);

#include <vulkan/vulkan.h>

#include <string>
#include <assert.h>
#include <array>

namespace forge
{
	struct ForgeBuffer;
	struct ForgeFrame;
	struct ForgeDynamicMemory;

	static constexpr uint32_t FORGE_MAX_OFF_SCREEN_FRAMES = 16u;

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
		VkCommandPool command_pool;

		ForgeBuffer* staging_buffer;
		VkCommandBuffer staging_command_buffer;

		ForgeDynamicMemory* uniform_memory;

		VkDebugUtilsMessengerEXT debug_messenger;
		PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT;
		PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT;
		PFN_vkCmdBeginDebugUtilsLabelEXT pfn_vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT pfn_vkCmdEndDebugUtilsLabelEXT;

		ForgeFrame* swapchain_frame;
		VkSemaphore swapchain_rendering_done;
		uint64_t swapchain_next_signal;

		ForgeFrame* offscreen_frames[FORGE_MAX_OFF_SCREEN_FRAMES];
		VkSemaphore offscreen_rendering_done;
		uint32_t offscreen_frames_count;
		uint64_t offscreen_next_signal;
	};

	Forge*
	forge_new();

	void
	forge_destroy(Forge* forge);

	void
	forge_flush(Forge* forge);
};