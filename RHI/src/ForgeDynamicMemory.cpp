#include "Forge.h"
#include "ForgeDynamicMemory.h"
#include "ForgeBuffer.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"

namespace forge
{
	static uint32_t
	_forge_dynamic_memory_acquire_available_segment(Forge* forge, ForgeDynamicMemory* memory)
	{
		// TODO:
	}

	static bool
	_froge_dynamic_memory_init(Forge* forge, uint32_t size, VkBufferUsageFlags usage, ForgeDynamicMemory* memory)
	{
		ForgeBufferDescription desc {};
		desc.name = "Dynamic memory";
		desc.size = size;
		desc.usage = usage;
		desc.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		memory->buffer = forge_buffer_new(forge, desc);

		if (memory->buffer == nullptr)
		{
			log_error("Failed to initialize dynamic memory");
			return false;
		}

		memory->current = 0;
		memory->segment_size = size / FORGE_DYNAMIC_MEMORY_MAX_SEGMENTTS;

		for (uint32_t i = 0; i < FORGE_DYNAMIC_MEMORY_MAX_SEGMENTTS; ++i)
		{
			memory->cursor[i] = 0u;
			memory->release_signal[i] = 0u;
		}

		return true;
	}

	static void
	_forge_dynamic_memory_free(Forge* forge, ForgeDynamicMemory* memory)
	{
		if (memory->buffer)
		{
			forge_buffer_destroy(forge, memory->buffer);
		}
	}

	ForgeDynamicMemory*
	forge_dynamic_memory_new(Forge* forge, uint32_t size, VkBufferUsageFlags usage)
	{
		auto memory = new ForgeDynamicMemory();

		if (_froge_dynamic_memory_init(forge, size, usage, memory) == false)
		{
			forge_dynamic_memory_destroy(forge, memory);
			return nullptr;
		}

		return memory;
	}

	uint64_t
	forge_dynamic_memory_write(Forge* forge, ForgeDynamicMemory* memory, uint32_t size, uint32_t alignment, void* data)
	{
		auto buffer = memory->buffer;
		uint32_t& current = memory->current;

		size = _forge_align_up(size, alignment);
		if ((memory->cursor[current] + size) > memory->segment_size)
		{
			// TODO: Handle running out of memory
			log_warning("Dynamic memory has run out of memory, potential memory ovwerwrite");
		}

		VkDeviceSize offset = _forge_align_up(current * memory->segment_size, alignment);
		offset += memory->cursor[current];

		memory->cursor[current] += size;

		void* dst = (char*)memory->buffer->mapped_ptr + offset;
		memcpy(dst, data, size);

		return offset;
	}

	void
	forge_dynamic_memory_flush(Forge* forge, ForgeDynamicMemory* memory)
	{
		// TODO?: Dynamic memory should be host coherent so we shouldn't need to flush
	}

	void
	forge_dynamic_memory_destroy(Forge* forge, ForgeDynamicMemory* memory)
	{
		if (memory)
		{
			_forge_dynamic_memory_free(forge, memory);
			delete memory;
		}
	}
};