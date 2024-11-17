#include "ForgeImage.h"
#include "Forge.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"

namespace forge
{
    static bool
    _forge_image_init(Forge* forge, ForgeImage* image)
    {
        VkResult res;

        const auto& desc = image->descriptrion;

        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = desc.type;
        image_info.extent = desc.extent;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = desc.format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = desc.usage;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        res = vkCreateImage(forge->device, &image_info, nullptr, &image->handle);
        VK_RES_CHECK(res);

        if (res != VK_SUCCESS)
        {
            log_error("Failed to create image '{}'", desc.name);
            return false;
        }

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(forge->device, image->handle, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = _find_memory_type(forge, mem_requirements.memoryTypeBits, desc.memory_properties);

        VkDeviceMemory memory;
        res = vkAllocateMemory(forge->device, &alloc_info, nullptr, &memory);
        VK_RES_CHECK(res);

        if (res != VK_SUCCESS)
        {
            log_error("Failed to allocate memory for image '{}'", desc.name);
            return false;
        }

        res = vkBindImageMemory(forge->device, image->handle, memory, 0);
        VK_RES_CHECK(res);

        VkImageViewCreateInfo view_info {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image->handle;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = desc.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        res = vkCreateImageView(forge->device, &view_info, nullptr, &image->view);
        VK_RES_CHECK(res);

        if (res != VK_SUCCESS)
        {
            log_error("Failed to create image view for '{}'", desc.name);
            return false;
        }

        if (desc.name.empty() == false)
        {
            _forge_debug_obj_name_set(forge, (uint64_t)image->handle, VK_OBJECT_TYPE_IMAGE, desc.name.c_str());
        }

        log_info("Image '{}' initialized successfully", desc.name);

        return true;
    }

    static void
    _forge_image_free(Forge* forge, ForgeImage* image)
    {
        if (image->view)
        {
            vkDestroyImageView(forge->device, image->view, nullptr);
        }

        if (image->handle)
        {
            vkDestroyImage(forge->device, image->handle, nullptr);
        }
    }

    ForgeImage*
    forge_image_new(Forge* forge, ForgeImageDescription descriptrion)
    {
        auto image = new ForgeImage;
        image->descriptrion = descriptrion;

        if (!_forge_image_init(forge, image))
        {
            forge_image_destroy(forge, image);
            return nullptr;
        }

        return image;
    }

    void
    forge_image_write(Forge* forge, ForgeImage* image, ForgeImageData data)
    {
        // Implement writing to an image using a staging buffer
        log_warning("forge_image_write: Not implemented yet");
    }

    void
    forge_image_destroy(Forge* forge, ForgeImage* image)
    {
        if (image)
        {
            _forge_image_free(forge, image);
            delete image;
        }
    }
}