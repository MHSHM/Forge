#ifdef _WIN32
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "Forge.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeBuffer.h"

#include <vulkan/vulkan_win32.h>

#include <vector>

namespace forge
{
	static constexpr uint32_t STAGING_BUFFER_SIZE = 64000u;

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
	_forge_command_pool_init(Forge* forge)
	{
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = forge->queue_family_index;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		auto res = vkCreateCommandPool(forge->device, &info, nullptr, &forge->command_pool);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create the global command pool");
			return false;
		}

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

		log_info("Global staging buffer was creating successfully");

		VkCommandBufferAllocateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1u;
		info.commandPool = forge->command_pool;
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
	_forge_init(Forge* forge)
	{
		if (_forge_instance_init(forge) == false)
		{
			return false;
		}

		if (_forge_physical_device_init(forge) == false)
		{
			return false;
		}

		if (_forge_logical_device_init(forge) == false)
		{
			return false;
		}

		if (_forge_debug_messenger_init(forge) == false)
		{
			return false;
		}

		if (_forge_command_pool_init(forge) == false)
		{
			return false;
		}

		if (_forge_staging_buffer_init(forge) == false)
		{
			return false;
		}

		return true;
	}

	static void
	_forge_free(Forge* forge)
	{
		if (forge->debug_messenger)
		{
			forge->pfn_vkDestroyDebugUtilsMessengerEXT(forge->instance, forge->debug_messenger, nullptr);
		}

		if (forge->device)
		{
			vkDestroyDevice(forge->device, nullptr);
		}

		if (forge->instance)
		{
			vkDestroyInstance(forge->instance, nullptr);
		}
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
};