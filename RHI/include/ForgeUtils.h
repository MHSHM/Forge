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
	static uint64_t
	_forge_align_up(uint64_t value, uint64_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	static uint64_t
	_forge_align_down(uint64_t value, uint64_t alignment)
	{
		return (value) & ~(alignment - 1);
	}

	template <class T>
	static void
	_forge_hash_combine(uint64_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
	}

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

	static uint32_t
	_forge_format_size(VkFormat format)
	{
		switch (format)
		{
			// 1-component formats
			case VK_FORMAT_R8_UNORM:
			case VK_FORMAT_R8_SNORM:
			case VK_FORMAT_R8_UINT:
			case VK_FORMAT_R8_SINT:
				return 1; // 8 bits = 1 byte

			// 2-component formats
			case VK_FORMAT_R8G8_UNORM:
			case VK_FORMAT_R8G8_SNORM:
			case VK_FORMAT_R8G8_UINT:
			case VK_FORMAT_R8G8_SINT:
				return 2; // 16 bits = 2 bytes

			// 4-component formats (RGBA)
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_SNORM:
			case VK_FORMAT_R8G8B8A8_UINT:
			case VK_FORMAT_R8G8B8A8_SINT:
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_SNORM:
			case VK_FORMAT_B8G8R8A8_UINT:
			case VK_FORMAT_B8G8R8A8_SINT:
				return 4; // 32 bits = 4 bytes

			// 16-bit float formats
			case VK_FORMAT_R16_SFLOAT:
				return 2; // 16 bits = 2 bytes

			case VK_FORMAT_R16G16_SFLOAT:
				return 4; // 16 bits x 2 = 4 bytes

			case VK_FORMAT_R16G16B16A16_SFLOAT:
				return 8; // 16 bits x 4 = 8 bytes

			// 32-bit float formats
			case VK_FORMAT_R32_SFLOAT:
				return 4; // 32 bits = 4 bytes

			case VK_FORMAT_R32G32_SFLOAT:
				return 8; // 32 bits x 2 = 8 bytes

			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return 16; // 32 bits x 4 = 16 bytes

			// Depth-stencil formats
			case VK_FORMAT_D16_UNORM:
				return 2; // 16 bits = 2 bytes

			case VK_FORMAT_D32_SFLOAT:
				return 4; // 32 bits = 4 bytes

			case VK_FORMAT_D24_UNORM_S8_UINT:
				return 4; // 24 bits depth + 8 bits stencil = 4 bytes

			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return 5; // 32 bits depth + 8 bits stencil = 5 bytes

			default:
				log_error("Unhandled format");
				assert(false);
		}

		return 0;
	}

	static VkImageAspectFlags
	_forge_image_aspect(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		default:
			log_error("Unhandled format");
			assert(false);
			break;
		}

		return 0u;
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