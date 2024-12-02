#include "Forge.h"
#include "ForgeSwapchain.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"

namespace forge
{
	static bool
	_forge_swapchain_init(Forge* forge, ForgeSwapchain* swapchain)
	{
		VkResult res;

		auto& description = swapchain->description;

		if (description.window == nullptr)
		{
			log_error("Invalid window handle provided for swapchain creation");
			return false;
		}

#ifdef VK_USE_PLATFORM_WIN32_KHR
		VkWin32SurfaceCreateInfoKHR surface_info{};
		surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_info.hinstance = GetModuleHandle(nullptr);
		surface_info.hwnd = (HWND)description.window;
		res = vkCreateWin32SurfaceKHR(forge->instance, &surface_info, nullptr, &swapchain->surface);
#else
		log_error("Unsupported platform");
		return false;
#endif
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize the surface");
			return false;
		}
		_forge_debug_obj_name_set(forge, (uint64_t)swapchain->surface, VK_OBJECT_TYPE_SURFACE_KHR, "Surface");

		log_info("Surface was created successfully");

		VkSurfaceCapabilitiesKHR surface_capabilities;
		res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(forge->physical_device, swapchain->surface, &surface_capabilities);
		VK_RES_CHECK(res);

		if (description.images_count < surface_capabilities.minImageCount)
		{
			log_error("Swapchain must at least have one presentable image");
			return false;
		}

		if (surface_capabilities.maxImageCount > 0 && description.images_count > surface_capabilities.maxImageCount)
		{
			log_warning(
				"The requested number of swapchain images '{}' exceeds the maximum number supported by the surface '{}', the swapchain will be created with '{}' images",
				description.images_count, surface_capabilities.maxImageCount
			);

			description.images_count = surface_capabilities.maxImageCount;
		}

		if (_forge_surface_present_mode_support(forge, swapchain->surface, description.present_mode))
		{
			log_info(
				"Required presentation mode '{}' is supported",
				_forge_swapchain_present_mode_to_str(description.present_mode)
			);
		}
		else
		{
			log_warning(
				"Required presentation mode '{}' is not supported, falling back to '{}'",
				_forge_swapchain_present_mode_to_str(description.present_mode),
				_forge_swapchain_present_mode_to_str(VK_PRESENT_MODE_FIFO_KHR)
			);

			description.present_mode = VK_PRESENT_MODE_FIFO_KHR;
		}

		VkSurfaceFormatKHR surface_format = { description.format, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		if (_forge_surface_format_support(forge, swapchain->surface, surface_format))
		{
			log_info(
				"Required format '{}' is supported",
				_forge_surface_format_to_str(surface_format.format)
			);
			swapchain->color_space = surface_format.colorSpace;
		}
		else
		{
			log_error(
				"Required format '{}' is not supported",
				_forge_surface_format_to_str(surface_format.format)
			);

			return false;
		}

		VkSwapchainCreateInfoKHR swapchain_info {};
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.surface = swapchain->surface;
		swapchain_info.minImageCount = description.images_count;
		swapchain_info.imageFormat = description.format;
		swapchain_info.imageColorSpace = surface_format.colorSpace;
		swapchain_info.imageExtent = description.extent;
		swapchain_info.imageArrayLayers = 1u;
		swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_info.presentMode = description.present_mode;
		swapchain_info.clipped = VK_TRUE;
		swapchain_info.oldSwapchain = VK_NULL_HANDLE;
		res = vkCreateSwapchainKHR(forge->device, &swapchain_info, nullptr, &swapchain->handle);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error(
				"Failed to create the swapchain, the following error code '{}' is reported",
				_forge_result_to_str(res)
			);

			return false;
		}
		_forge_debug_obj_name_set(forge, (uint64_t)swapchain->handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "Swapchain");

		swapchain->images.resize(description.images_count);
		res = vkGetSwapchainImagesKHR(forge->device, swapchain->handle, &description.images_count, swapchain->images.data());
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to get the swapchain images, the following error code '{}' is reported",
				_forge_result_to_str(res)
			);

			return false;
		}

		for (uint32_t i = 0; i < FORGE_SWAPCHIAN_INFLIGH_FRAMES; ++i)
		{
			VkSemaphoreCreateInfo semaphore_info {};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			res = vkCreateSemaphore(forge->device, &semaphore_info, nullptr, &swapchain->image_available[i]);
			VK_RES_CHECK(res);

			if (res != VK_SUCCESS)
			{
				log_error("Failed to create the image available semaphore");
				return false;
			}

			res = vkCreateSemaphore(forge->device, &semaphore_info, nullptr, &swapchain->rendering_done[i]);
			VK_RES_CHECK(res);

			if (res != VK_SUCCESS)
			{
				log_error("Failed to create the rendering done semaphore");
				return false;
			}
		}

		log_info("Swapchain was created successfully");

		return true;
	}

	static bool
	_forge_swapchain_update(Forge* forge, ForgeSwapchain* swapchain, uint32_t width, uint32_t height)
	{
		VkResult res;

		auto old_swapchain = swapchain->handle;

		VkSwapchainCreateInfoKHR swapchain_info{};
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.surface = swapchain->surface;
		swapchain_info.minImageCount = swapchain->description.images_count;
		swapchain_info.imageFormat = swapchain->description.format;
		swapchain_info.imageColorSpace = swapchain->color_space;
		swapchain_info.imageExtent = { width, height };
		swapchain_info.imageArrayLayers = 1u;
		swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_info.presentMode = swapchain->description.present_mode;
		swapchain_info.clipped = VK_TRUE;
		swapchain_info.oldSwapchain = old_swapchain;
		res = vkCreateSwapchainKHR(forge->device, &swapchain_info, nullptr, &swapchain->handle);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to update the swapchain");
			return false;
		}

		// TODO: Add a deletion queue
		vkDeviceWaitIdle(forge->device);
		vkDestroySwapchainKHR(forge->device, old_swapchain, nullptr);

		swapchain->images.clear();
		swapchain->images.resize(swapchain->description.images_count);
		res = vkGetSwapchainImagesKHR(forge->device, swapchain->handle, &swapchain->description.images_count, swapchain->images.data());
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error(
				"Failed to update the swapchain, unable to get the new images, the following error code '{}' is reported",
				_forge_result_to_str(res)
			);

			return false;
		}

		log_info("Swapchain was updated successfully, the new dimensions are {}x{}", width, height);

		return true;
	}

	static void
	_forge_swapchain_free(Forge* forge, ForgeSwapchain* swapchain)
	{
		if (swapchain->handle)
		{
			vkDestroySwapchainKHR(forge->device, swapchain->handle, nullptr);
		}

		if (swapchain->surface)
		{
			vkDestroySurfaceKHR(forge->instance, swapchain->surface, nullptr);
		}

		swapchain->images.clear();
	}

	ForgeSwapchain*
	forge_swapchain_new(Forge* forge, ForgeSwapchainDescription description)
	{
		auto swapchain = new ForgeSwapchain();
		swapchain->description = description;

		if (_forge_swapchain_init(forge, swapchain) == false)
		{
			forge_swapchain_destroy(forge, swapchain);
			return nullptr;
		}

		return swapchain;
	}

	bool
	forge_swapchain_update(Forge* forge, ForgeSwapchain* swapchain)
	{
		if (swapchain)
		{
			VkSurfaceCapabilitiesKHR capabilities;
			auto res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(forge->physical_device, swapchain->surface, &capabilities);
			VK_RES_CHECK(res);

			auto current_width = capabilities.currentExtent.width;
			auto current_height = capabilities.currentExtent.height;
			if (current_width == 0 || current_height == 0)
			{
				return false;
			}

			auto width = swapchain->description.extent.width;
			auto height = swapchain->description.extent.height;

			if (current_width != width || current_height != height)
			{
				if (_forge_swapchain_update(forge, swapchain, current_width, current_height))
				{
					swapchain->description.extent.width = current_width;
					swapchain->description.extent.height = current_height;

					return true;
				}
			}
		}

		return false;
	}

	void
	forge_swapchain_destroy(Forge* forge, ForgeSwapchain* swapchain)
	{
		if (swapchain)
		{
			_forge_swapchain_free(forge, swapchain);
			delete swapchain;
		}
	}
};