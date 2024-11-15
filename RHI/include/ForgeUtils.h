#pragma once

#define VK_RES_CHECK(res) assert(res == VK_SUCCESS);

#include "ForgeLogger.h"

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

	bool
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
};