#include "Forge.h"
#include "ForgeBuffer.h"
#include "ForgeUtils.h"

namespace forge
{
	static bool
	_forge_buffer_init(Forge* forge, ForgeBuffer* buffer, void* data, uint32_t size)
	{
		VkResult res;

		if (buffer->description.size <= 0)
		{
			log_error("You can't create an empty buffer");
			return false;
		}

		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = buffer->description.size;
		buffer_info.usage = buffer->description.usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		res = vkCreateBuffer(forge->device, &buffer_info, nullptr, &buffer->handle);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create buffer '{}', the following error code '{}' is reported", buffer->description.name, _forge_result_to_str(res));
			return false;
		}

		if (buffer->description.name.empty() == false)
		{
			_forge_debug_obj_name_set(forge, (uint64_t)buffer->handle, VK_OBJECT_TYPE_BUFFER, buffer->description.name.c_str());
		}

		VkMemoryRequirements mem_requirements;
		vkGetBufferMemoryRequirements(forge->device, buffer->handle, &mem_requirements);

		VkMemoryAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = _find_memory_type(forge, mem_requirements.memoryTypeBits, buffer->description.memory_properties);
		res = vkAllocateMemory(forge->device, &alloc_info, nullptr, &buffer->memory);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to allocate memory for the buffer '{}', the following error code '{}' is reported", buffer->description.name, _forge_result_to_str(res));
			return false;
		}

		res = vkBindBufferMemory(forge->device, buffer->handle, buffer->memory, 0);
		VK_RES_CHECK(res);

		if (buffer->description.memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			res = vkMapMemory(forge->device, buffer->memory, 0u, mem_requirements.size, 0u, &buffer->mapped_ptr);
			VK_RES_CHECK(res);
		}

		buffer->description.size = mem_requirements.size;

		return true;
	}

	static void
	_forge_buffer_write(Forge* forge, ForgeBuffer* buffer, void* data, uint32_t size)
	{
		if (buffer->description.memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			memcpy(buffer->mapped_ptr, data, size);

			if ((buffer->description.memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange range{};
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.memory = buffer->memory;
				range.offset = 0u;
				range.size = size;
				auto res = vkFlushMappedMemoryRanges(forge->device, 1u, &range);
				VK_RES_CHECK(res);
			}
		}
		else
		{
		
		}
	}

	static void
	_forge_buffer_free(Forge* forge, ForgeBuffer* buffer)
	{
		if (buffer->handle)
		{
			vkDestroyBuffer(forge->device, buffer->handle, nullptr);
		}

		if (buffer->memory)
		{
			vkFreeMemory(forge->device, buffer->memory, nullptr);
		}
	}

	ForgeBuffer*
	forge_buffer_new(Forge* forge, ForgeBufferDescription descriptrion, void* data, uint32_t size)
	{
		auto buffer = new ForgeBuffer();
		buffer->description = descriptrion;

		if (_forge_buffer_init(forge, buffer, data, size) == false)
		{
			forge_buffer_destroy(forge, buffer);
			return nullptr;
		}

		return buffer;
	}

	void
	forge_buffer_write(Forge* forge, ForgeBuffer* buffer, void* data, uint32_t size)
	{
		if (data == nullptr) { log_warning("forge_buffer_write: The provided data is empty, skipping the write operation"); return; }
		if (size == 0u) { log_warning("forge_buffer_write: The provided size is 0, skipping the write operation"); return; }
		if (size > buffer->description.size) { log_error("forge_buffer_write: The provided size exceeds the buffer size, skipping the write operation"); return; }

		_forge_buffer_write(forge, buffer, data, size);
	}

	void
	forge_buffer_destroy(Forge* forge, ForgeBuffer* buffer)
	{
		if (buffer)
		{
			_forge_buffer_free(forge, buffer);
			delete buffer;
		}
	}
};