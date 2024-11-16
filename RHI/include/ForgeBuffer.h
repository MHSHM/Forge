#pragma once

#include <vulkan/vulkan.h>

namespace forge
{
	struct Forge;

	struct ForgeBufferDescription
	{
		std::string name;
		uint32_t size;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags memory_properties;
	};

	struct ForgeBuffer
	{
		VkBuffer handle;
		VkDeviceMemory memory;
		void* mapped_ptr;
		uint32_t cursor;
		ForgeBufferDescription description;
	};

	ForgeBuffer*
	forge_buffer_new(Forge* forge, ForgeBufferDescription descriptrion, void* data, uint32_t size);

	void
	forge_buffer_write(Forge* forge, ForgeBuffer* buffer, void* data, uint32_t size);

	void
	forge_buffer_destroy(Forge* forge, ForgeBuffer* buffer);
};