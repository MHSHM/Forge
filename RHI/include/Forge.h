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
	struct ForgeDeletionQueue;
	struct ForgeDescriptorSetManager;
	struct ForgeCommandBufferManager;

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

		ForgeBuffer* staging_buffer;
		VkCommandBuffer staging_command_buffer;

		ForgeDynamicMemory* uniform_memory;
		ForgeDeletionQueue* deletion_queue;
		ForgeDescriptorSetManager* descriptor_set_manager;
		ForgeCommandBufferManager* command_buffer_manager;

		VkDebugUtilsMessengerEXT debug_messenger;
		PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT;
		PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT;
		PFN_vkCmdBeginDebugUtilsLabelEXT pfn_vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT pfn_vkCmdEndDebugUtilsLabelEXT;

		ForgeFrame* swapchain_frame;
		ForgeFrame* offscreen_frames[FORGE_MAX_OFF_SCREEN_FRAMES];
		uint32_t offscreen_frames_count;

		VkSemaphore timeline;
		uint64_t timeline_next_signal;
		uint64_t timeline_current_signal;
	};

	Forge*
	forge_new();

	void
	forge_destroy(Forge* forge);

	void
	forge_flush(Forge* forge);
};