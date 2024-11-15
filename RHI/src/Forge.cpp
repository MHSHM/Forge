#ifdef _WIN32
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_RES_CHECK(res) assert(res == VK_SUCCESS);

#include "Forge.h"
#include "ForgeLogger.h"

#include <vulkan/vulkan_win32.h>

#include <assert.h>
#include <vector>

namespace forge
{
	inline static const char*
	_forge_vulk_result_to_str(VkResult res)
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

		log_warning("Provided error code is not handled here\n");
		return "";
	}

	static const char* _forge_vulk_api_version_to_str(uint32_t api_version)
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

	static bool _forge_vulk_instance_extension_support(const char* extension)
	{
		uint32_t extensions_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);

		std::vector<VkExtensionProperties> extensions(extensions_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, extensions.data());

		for (auto& supported_extension : extensions)
		{
			if (strcmp(extension, supported_extension.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	static bool _forge_vulk_instance_init(Forge* forge)
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
		const char** extensions = nullptr;
		const char* extensions_list[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		};
		extensions_count = sizeof(extensions_list) / sizeof(extensions_list[0]);
		extensions = extensions_list;

		uint32_t supported_version = VK_API_VERSION_1_0;
		vkEnumerateInstanceVersion(&supported_version);
		if (supported_version < VK_API_VERSION_1_2)
		{
			log_error(
				"Instance version '{}' doesn't meet the minimum required version '{}'",
				_forge_vulk_api_version_to_str(supported_version),
				_forge_vulk_api_version_to_str(VK_API_VERSION_1_2)
			);

			return false;
		}

		for (uint32_t i = 0; i < extensions_count; ++i)
		{
			if (_forge_vulk_instance_extension_support(extensions[i]) == false)
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
				_forge_vulk_result_to_str(res)
			);

			return false;
		}

		log_info("Vulkan instance was created successfully");

		return true;
	}

	static bool _forge_init(Forge* forge)
	{
		if (_forge_vulk_instance_init(forge) == false)
		{
			return false;
		}

		return true;
	}

	static void _forge_free(Forge* forge)
	{
		if (forge->instance)
		{
			vkDestroyInstance(forge->instance, nullptr);
		}
	}

	Forge* forge_new()
	{
		auto forge = new Forge;
		bool init = _forge_init(forge);
		if (init == false)
		{
			forge_destroy(forge);
			return  nullptr;
		}

		return forge;
	}

	void forge_destroy(Forge* forge)
	{
		if (forge)
		{
			_forge_free(forge);
			delete forge;
		}
	}
};