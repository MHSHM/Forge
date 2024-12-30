#ifdef _WIN32
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "Forge.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeBuffer.h"
#include "ForgeDynamicMemory.h"
#include "ForgeFrame.h"
#include "ForgeDeletionQueue.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeCommandBufferManager.h"

#include <vulkan/vulkan_win32.h>

#include <vector>

namespace forge
{
	static constexpr uint32_t STAGING_BUFFER_SIZE = 32 << 20;
	static constexpr uint32_t UNIFORM_MEMORY_SIZE = 16 << 20;

	static bool
	_forge_instance_init(Forge* forge)
	{
		VkResult res;

		// TODO: Check support for required layers
		uint32_t layers_count = 0u;
		const char** layers = nullptr;
	#ifndef NDEBUG
		const char* layers_list[] = {
			"VK_LAYER_KHRONOS_validation"
		};
		layers_count = sizeof(layers_list) / sizeof(layers_list[0]);
		layers = layers_list;
	#endif

		uint32_t extensions_count = 0u;
		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		extensions_count = sizeof(extensions) / sizeof(extensions[0]);

		uint32_t supported_version = VK_API_VERSION_1_0;
		vkEnumerateInstanceVersion(&supported_version);
		if (supported_version < VK_API_VERSION_1_2)
		{
			log_error(
				"Instance version '{}' doesn't meet the minimum required version '{}'",
				_forge_api_version_to_str(supported_version),
				_forge_api_version_to_str(VK_API_VERSION_1_2)
			);

			return false;
		}

		for (uint32_t i = 0; i < extensions_count; ++i)
		{
			if (_forge_instance_extension_support(extensions[i]) == false)
			{
				log_error("Required instance extension '{}' is not supported", extensions[i]);

				return false;
			}

			log_info("Required instance extension '{}' is supported", extensions[i]);
		}

		VkApplicationInfo app_info {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Forge Application";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Forge Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = supported_version;

		VkInstanceCreateInfo instance_info{};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;
		instance_info.enabledLayerCount = layers_count;
		instance_info.ppEnabledLayerNames = layers;
		instance_info.enabledExtensionCount = extensions_count;
		instance_info.ppEnabledExtensionNames = extensions;
		res = vkCreateInstance(&instance_info, nullptr, &forge->instance);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error(
				"Failed to create the instance, the following error code '{}' is reported",
				_forge_result_to_str(res)
			);

			return false;
		}

		log_info("Vulkan instance was created successfully");

		return true;
	}

	static bool
	_forge_physical_device_init(Forge* forge)
	{
		VkResult res;

		uint32_t count = 0;
		res = vkEnumeratePhysicalDevices(forge->instance, &count, nullptr);
		VK_RES_CHECK(res);

		if (count == 0)
		{
			log_error("No physical devices found that support Vulkan");
			return false;
		}

		std::vector<VkPhysicalDevice> physical_devices(count);
		res = vkEnumeratePhysicalDevices(forge->instance, &count, physical_devices.data());
		VK_RES_CHECK(res);

		VkPhysicalDeviceProperties properties{};
		VkPhysicalDevice chosen_device = VK_NULL_HANDLE;
		uint32_t chosen_queue_family_index = 0;
		std::string chose_device_name {};

		for (uint32_t i = 0; i < count; ++i)
		{
			const auto& physical_device = physical_devices[i];
			vkGetPhysicalDeviceProperties(physical_device, &properties);
			if (properties.apiVersion < VK_API_VERSION_1_2)
			{
				continue;
			}

			uint32_t queue_families_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);

			std::vector<VkQueueFamilyProperties> queue_families_properties(queue_families_count);
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_families_properties.data());

			for (uint32_t j = 0; j < queue_families_count; ++j)
			{
				const auto& queue_family_properties = queue_families_properties[j];

				if (queue_family_properties.queueCount > 0 &&
					queue_family_properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
				{
					VkBool32 supports_present;
				#ifdef VK_USE_PLATFORM_WIN32_KHR
					supports_present = vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, j);
				#elif
					log_error("Platform is not supported");
					return false;
				#endif

					if (supports_present)
					{

						chosen_device = physical_device;
						chose_device_name = std::string(properties.deviceName);
						chosen_queue_family_index = j;
						break;
					}
				}
			}

			if (chosen_device != VK_NULL_HANDLE && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				break;
			}
		}

		if (chosen_device == VK_NULL_HANDLE)
		{
			log_error("Failed to find a suitable physical device that meets the minimum requirements");
			return false;
		}

		VkPhysicalDeviceMemoryProperties memory_properties{};
		vkGetPhysicalDeviceMemoryProperties(chosen_device, &memory_properties);
		uint64_t memory = 0u;

		// TODO: Is this the correct way to get the VRAM of the device?
		for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
		{
			const auto& memory_type = memory_properties.memoryTypes[i];

			if ((memory_type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
			{
				memory = memory_properties.memoryHeaps[memory_type.heapIndex].size;
			}
		}

		forge->physical_device = chosen_device;
		forge->physical_device_memory_properties = memory_properties;
		forge->physical_device_limits = properties.limits;
		forge->physical_device_name = chose_device_name;
		forge->physical_memory = memory;
		forge->queue_family_index = chosen_queue_family_index;

		log_info("Physical device '{}' was picked successfully and it has '{}' Gigabytes of memory", forge->physical_device_name, forge->physical_memory / (uint64_t)1e+9);

		return true;
	}

	static bool
	_forge_logical_device_init(Forge* forge)
	{
		VkResult res;

		uint32_t extensions_count = 0;
		const char* extensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		extensions_count = sizeof(extensions) / sizeof(extensions[0]);

		for (uint32_t i = 0; i < extensions_count; ++i)
		{
			if (_forge_device_extension_support(forge, extensions[i]) == false)
			{
				log_error("Required device extension '{}' is not supported", extensions[i]);
				return false;
			}

			log_info("Required device extension '{}' is supported", extensions[i]);
		}

		float queue_priorites[] = { 1.0f };

		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = forge->queue_family_index;
		queue_info.queueCount = 1u;
		queue_info.pQueuePriorities = queue_priorites;

		VkPhysicalDeviceFeatures device_features {};

		// Enable timeline semaphore feature
		VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features {};
		timeline_semaphore_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timeline_semaphore_features.timelineSemaphore = VK_TRUE;

		VkDeviceCreateInfo device_info{};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = 1u;
		device_info.pQueueCreateInfos = &queue_info;
		device_info.enabledExtensionCount = extensions_count;
		device_info.ppEnabledExtensionNames = extensions;
		device_info.pEnabledFeatures = &device_features;
		device_info.pNext = &timeline_semaphore_features;
		res = vkCreateDevice(forge->physical_device, &device_info, nullptr, &forge->device);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error(
				"Failed to create the device, the following error code '{}' is reported",
				_forge_result_to_str(res)
			);
			return false;
		}

		vkGetDeviceQueue(forge->device, forge->queue_family_index, 0u, &forge->queue);

		log_info("Device and Queue were created successfully");

		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL
	debug_call_back(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			log_error("{}", pCallbackData->pMessage);
		}
		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			log_warning("{}", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	static bool
	_forge_debug_messenger_init(Forge* forge)
	{
		VkResult res;

		VkDebugUtilsMessengerCreateInfoEXT create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = debug_call_back;

		forge->pfn_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(forge->instance, "vkCreateDebugUtilsMessengerEXT");
		forge->pfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(forge->instance, "vkDestroyDebugUtilsMessengerEXT");

		forge->pfn_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(forge->device, "vkSetDebugUtilsObjectNameEXT");
		forge->pfn_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(forge->device, "vkCmdBeginDebugUtilsLabelEXT");
		forge->pfn_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(forge->device, "vkCmdEndDebugUtilsLabelEXT");

		res = forge->pfn_vkCreateDebugUtilsMessengerEXT(forge->instance, &create_info, nullptr, &forge->debug_messenger);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create the debug messenger");
			return false;
		}

		log_info("Debug messenger was created successfully");

		return true;
	}

	static bool
	_forge_staging_buffer_init(Forge* forge)
	{
		ForgeBufferDescription desc {};
		desc.name = "Forge staging buffer";
		desc.size = STAGING_BUFFER_SIZE;
		desc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		desc.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		forge->staging_buffer = forge_buffer_new(forge, desc);
		if (forge->staging_buffer == nullptr)
		{
			return false;
		}

		log_info("A '{}' bytes of global staging memory was created successfully", STAGING_BUFFER_SIZE);

		VkCommandBufferAllocateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1u;
		info.commandPool = forge->command_buffer_manager->pool;
		auto res = vkAllocateCommandBuffers(forge->device, &info, &forge->staging_command_buffer);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create the global staging command buffer");
			return false;
		}

		return true;
	}

	static bool
	_forge_uniform_dynamic_memory_init(Forge* forge)
	{
		forge->uniform_memory = forge_dynamic_memory_new(forge, UNIFORM_MEMORY_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

		if (forge->uniform_memory == nullptr)
		{
			log_error("Failed to initialize uniform global dynamic memory");
			return false;
		}

		log_info("A '{}' bytes of global dynamic uniform memory was created successfully", UNIFORM_MEMORY_SIZE);

		return true;
	}

	static bool
	_forge_init(Forge* forge)
	{
		if (_forge_instance_init(forge) == false)
		{
			forge_destroy(forge);
			return false;
		}

		if (_forge_physical_device_init(forge) == false)
		{
			forge_destroy(forge);
			return false;
		}

		if (_forge_logical_device_init(forge) == false)
		{
			forge_destroy(forge);
			return false;
		}

		if (_forge_debug_messenger_init(forge) == false)
		{
			forge_destroy(forge);
			return false;
		}

		// FIXME: Need to be initialized before the staging command buffer
		forge->command_buffer_manager = forge_command_buffer_manager_new(forge);
		if (forge->command_buffer_manager == nullptr)
		{
			log_error("Failed to initialize the command buffer manager");
			forge_destroy(forge);
			return false;
		}

		if (_forge_staging_buffer_init(forge) == false)
		{
			forge_destroy(forge);
			return false;
		}

		if (_forge_uniform_dynamic_memory_init(forge) == false)
		{
			forge_destroy(forge);
			return false;
		}

		forge->deletion_queue = forge_deletion_queue_new(forge);
		if (forge->deletion_queue == nullptr)
		{
			log_error("Failed to initialize the deletion queue");
			forge_destroy(forge);
			return false;
		}

		forge->descriptor_set_manager = forge_descriptor_set_manager_new(forge);
		if (forge->descriptor_set_manager == nullptr)
		{
			log_error("Failed to initialize the descriptor set manager");
			forge_destroy(forge);
			return false;
		}

		VkSemaphoreTypeCreateInfo timeline_info {};
		timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timeline_info.initialValue = 0;

		VkSemaphoreCreateInfo rendering_done_info {};
		rendering_done_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		rendering_done_info.pNext = &timeline_info;
		rendering_done_info.flags = 0;
		auto res = vkCreateSemaphore(forge->device, &rendering_done_info, NULL, &forge->timeline);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize swapchain's rendering done semaphore");
			forge_destroy(forge);
			return false;
		}

		forge->timeline_next_check_point = 1u;

		return true;
	}

	static void
	_forge_free(Forge* forge)
	{
		if (forge->device)
		{
			vkDeviceWaitIdle(forge->device);
		}

		if (forge->timeline)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, forge->timeline);
		}

		if (forge->debug_messenger)
		{
			forge->pfn_vkDestroyDebugUtilsMessengerEXT(forge->instance, forge->debug_messenger, nullptr);
		}

		if (forge->command_buffer_manager)
		{
			forge_command_buffer_manager_destroy(forge, forge->command_buffer_manager);
		}

		if (forge->descriptor_set_manager)
		{
			forge_descriptor_set_manager_destroy(forge, forge->descriptor_set_manager);
		}

		if (forge->staging_buffer)
		{
			forge_buffer_destroy(forge, forge->staging_buffer);
		}

		if (forge->uniform_memory)
		{
			forge_dynamic_memory_destroy(forge, forge->uniform_memory);
		}

		forge_deletion_queue_flush(forge, forge->deletion_queue, true);
		forge_deletion_queue_destroy(forge, forge->deletion_queue);

		if (forge->device)
		{
			vkDestroyDevice(forge->device, nullptr);
		}

		if (forge->instance)
		{
			vkDestroyInstance(forge->instance, nullptr);
		}
	}

	static void
	_forge_frames_process(Forge* forge)
	{
		VkResult res;

		auto swapchain = forge->swapchain_frame->swapchain;
		auto index = swapchain->frame_index % FORGE_SWAPCHIAN_INFLIGH_FRAMES;

		VkCommandBuffer command_buffers[FORGE_MAX_OFF_SCREEN_FRAMES];
		uint32_t command_buffers_count = 0u;
		for (uint32_t i = 0; i < FORGE_MAX_OFF_SCREEN_FRAMES; ++i)
		{
			auto frame = forge->offscreen_frames[i];
			if (frame == nullptr)
				continue;

			command_buffers[command_buffers_count] = frame->command_buffer;

			++command_buffers_count;
		}
		command_buffers[command_buffers_count++] = forge->swapchain_frame->command_buffer;

		constexpr uint32_t MAX_SIGNAL_SEMAPHORES = 4u;
		VkSemaphore signal_semaphores[MAX_SIGNAL_SEMAPHORES]{};
		uint64_t signal_values[MAX_SIGNAL_SEMAPHORES]{};
		uint32_t signal_semaphores_count = 0u;

		signal_semaphores[signal_semaphores_count] = forge->timeline;
		signal_values[signal_semaphores_count++] = forge->timeline_next_check_point;

		signal_semaphores[signal_semaphores_count] = swapchain->rendering_done[index];
		signal_values[signal_semaphores_count++] = UINT64_MAX;

		constexpr uint32_t MAX_WAIT_SEMAPHORES = 4u;
		VkSemaphore wait_semaphores[MAX_WAIT_SEMAPHORES]{};
		uint64_t wait_values[MAX_WAIT_SEMAPHORES]{};
		VkPipelineStageFlags wait_stages[MAX_WAIT_SEMAPHORES] = {};
		uint32_t wait_semaphores_count = 0;

		wait_semaphores[wait_semaphores_count] = swapchain->image_available[index];
		wait_values[wait_semaphores_count] = UINT64_MAX;
		wait_stages[wait_semaphores_count++] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		VkTimelineSemaphoreSubmitInfo timeline_info {};
		timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		timeline_info.waitSemaphoreValueCount = wait_semaphores_count;
		timeline_info.pWaitSemaphoreValues = wait_values;
		timeline_info.signalSemaphoreValueCount = signal_semaphores_count;
		timeline_info.pSignalSemaphoreValues = signal_values;

		VkFence signal_fence = swapchain->fence[index];

		VkSubmitInfo submit_info {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = &timeline_info;
		submit_info.waitSemaphoreCount = wait_semaphores_count;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = command_buffers_count;
		submit_info.pCommandBuffers = command_buffers;
		submit_info.signalSemaphoreCount = signal_semaphores_count;
		submit_info.pSignalSemaphores = signal_semaphores;
		res = vkQueueSubmit(forge->queue, 1u, &submit_info, signal_fence);
		VK_RES_CHECK(res);

		VkPresentInfoKHR present_info {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1u;
		present_info.pWaitSemaphores = &swapchain->rendering_done[index];
		present_info.swapchainCount = 1u;
		present_info.pSwapchains = &swapchain->handle;
		present_info.pImageIndices = &swapchain->image_index;
		res = vkQueuePresentKHR(forge->queue, &present_info);
		VK_RES_CHECK(res);

		++forge->timeline_next_check_point;
		++swapchain->frame_index;
	}

	Forge*
	forge_new()
	{
		auto forge = new Forge();
		if (_forge_init(forge) == false)
		{
			forge_destroy(forge);
			return  nullptr;
		}

		return forge;
	}

	void
	forge_destroy(Forge* forge)
	{
		if (forge)
		{
			_forge_free(forge);
			delete forge;
		}
	}

	void
	forge_flush(Forge* forge)
	{
		_forge_frames_process(forge);
		forge_deletion_queue_flush(forge, forge->deletion_queue, false);
	}
};