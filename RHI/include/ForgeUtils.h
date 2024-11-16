#pragma once

#define VK_RES_CHECK(res) assert(res == VK_SUCCESS);

#include "Forge.h"
#include "ForgeLogger.h"
#include "ForgeSwapchain.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <assert.h>

namespace forge
{
	static const char*
	_forge_result_to_str(VkResult res)
	{
		switch (res)
		{
		case VK_SUCCESS:	return "VK_SUCCESS";
		case VK_NOT_READY:	return "VK_NOT_READY";
		case VK_TIMEOUT:	return "VK_TIMEOUT";
		case VK_EVENT_SET:	return "VK_EVENT_SET";
		case VK_EVENT_RESET:	return "VK_EVENT_RESET";
		case VK_INCOMPLETE:	return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:	return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:	return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:	return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:	return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:	return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:	return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:	return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:	return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:	return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:	return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:	return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:	return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:	return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY:	return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:	return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION:	return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:	return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_PIPELINE_COMPILE_REQUIRED:	return "VK_PIPELINE_COMPILE_REQUIRED";
		case VK_ERROR_SURFACE_LOST_KHR:	return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:	return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:	return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:	return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:	return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:	return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:	return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:	return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:	return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:	return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:	return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:	return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:	return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:	return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_KHR:	return "VK_ERROR_NOT_PERMITTED_KHR";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:	return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:	return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:	return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:	return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:	return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:	return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:	return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
		case VK_RESULT_MAX_ENUM:
		default:
			break;
		}

		log_warning("Provided error code '{}' is not handled here", res);
		return "";
	}

	static const char*
	_forge_api_version_to_str(uint32_t api_version)
	{
		switch (api_version)
		{
		case VK_API_VERSION_1_0:	return "1.0";
		case VK_API_VERSION_1_1:	return "1.1";
		case VK_API_VERSION_1_2:	return "1.2";
		case VK_API_VERSION_1_3:	return "1.3";
		default:
			break;
		}

		log_warning("Provided API version is not handled here\n");
		return "";
	}

	static bool
	_forge_instance_extension_support(const char* extension)
	{
		VkResult res;

		uint32_t extensions_count = 0;
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
		VK_RES_CHECK(res);

		std::vector<VkExtensionProperties> extensions(extensions_count);
		res = vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, extensions.data());
		VK_RES_CHECK(res);

		for (auto& supported_extension : extensions)
		{
			if (strcmp(extension, supported_extension.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	static bool
	_forge_device_extension_support(Forge* forge, const char* extension)
	{
		VkResult res;

		uint32_t extensions_count = 0;
		res = vkEnumerateDeviceExtensionProperties(forge->physical_device, nullptr, &extensions_count, nullptr);
		VK_RES_CHECK(res);

		std::vector<VkExtensionProperties> extensions(extensions_count);
		res = vkEnumerateDeviceExtensionProperties(forge->physical_device, nullptr, &extensions_count, extensions.data());
		VK_RES_CHECK(res);

		for (auto& supported_extension : extensions)
		{
			if (strcmp(extension, supported_extension.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	static bool
	_forge_surface_present_mode_support(Forge* forge, VkSurfaceKHR surface, VkPresentModeKHR mode)
	{
		VkResult res;

		uint32_t present_modes_count = 0;
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(forge->physical_device, surface, &present_modes_count, nullptr);
		VK_RES_CHECK(res);

		std::vector<VkPresentModeKHR> present_modes(present_modes_count);
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(forge->physical_device, surface, &present_modes_count, present_modes.data());
		VK_RES_CHECK(res);

		for (auto& supported_mode : present_modes)
		{
			if (supported_mode == mode)
			{
				return true;
			}
		}

		return false;
	}

	static bool
	_forge_surface_format_support(Forge* forge, VkSurfaceKHR surface, VkSurfaceFormatKHR format)
	{
		VkResult res;

		uint32_t formats_count = 0;
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(forge->physical_device, surface, &formats_count, nullptr);
		VK_RES_CHECK(res);

		std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(forge->physical_device, surface, &formats_count, surface_formats.data());
		VK_RES_CHECK(res);

		for (auto& supported_format : surface_formats)
		{
			if (supported_format.colorSpace == format.colorSpace && supported_format.format == format.format)
			{
				return true;
			}
		}

		return false;
	}

	static const char*
	_forge_surface_format_to_str(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8G8B8A8_UNORM:	return "VK_FORMAT_R8G8B8A8_UNORM";
		default:
			break;
		}

		log_warning("Provided format is not handled here\n");
		return "";
	}

	static const char*
	_forge_swapchain_present_mode_to_str(VkPresentModeKHR mode)
	{
		switch (mode)
		{
		case VK_PRESENT_MODE_IMMEDIATE_KHR:						return "VK_PRESENT_MODE_IMMEDIATE_KHR";
		case VK_PRESENT_MODE_MAILBOX_KHR:						return "VK_PRESENT_MODE_MAILBOX_KHR";
		case VK_PRESENT_MODE_FIFO_KHR:							return "VK_PRESENT_MODE_FIFO_KHR";
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR:					return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:			return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:		return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
		case VK_PRESENT_MODE_MAX_ENUM_KHR:
		default:
			break;
		}

		log_warning("Provided presentation mode is not handled here\n");
		return "";
	}

	static void
	_forge_debug_obj_name_set(Forge* forge, uint64_t handle, VkObjectType type, const char* name)
	{
		VkDebugUtilsObjectNameInfoEXT name_info{};
		name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		name_info.objectType = type;
		name_info.objectHandle = handle;
		name_info.pObjectName = name;
		auto res = forge->pfn_vkSetDebugUtilsObjectNameEXT(forge->device, &name_info);
		VK_RES_CHECK(res);
	}

	static void
	_forge_debug_begin_region(Forge* forge, VkCommandBuffer cmd_buffer, const char* region_name, float color[4])
	{
		VkDebugUtilsLabelEXT label_info{};
		label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label_info.pLabelName = region_name;
		memcpy(label_info.color, color, 4 * sizeof(float));
		forge->pfn_vkCmdBeginDebugUtilsLabelEXT(cmd_buffer, &label_info);
	}

	static void
	_forge_debug_end_region(Forge* forge, VkCommandBuffer cmd_buffer)
	{
		forge->pfn_vkCmdEndDebugUtilsLabelEXT(cmd_buffer);
	}

	static uint32_t
	_find_memory_type(Forge* forge, uint32_t type_filter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(forge->physical_device, &mem_properties);

		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
			if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		assert(false && "Failed to find suitable memory type!");
		return 0;
	}
};