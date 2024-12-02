#pragma once

#include <vulkan/vulkan.h>

namespace forge
{
	struct Forge;
	struct ForgeBuffer;

	static constexpr uint32_t FORGE_DYNAMIC_MEMORY_MAX_SEGMENTTS = 3u;

	struct ForgeDynamicMemory
	{
		ForgeBuffer* buffer;
		uint32_t segment_size;
		uint32_t cursor[FORGE_DYNAMIC_MEMORY_MAX_SEGMENTTS];
		uint32_t release_signal[FORGE_DYNAMIC_MEMORY_MAX_SEGMENTTS];
		uint32_t current;
	};

	ForgeDynamicMemory*
	forge_dynamic_memory_new(Forge* forge, uint32_t size, VkBufferUsageFlags usage);

	uint64_t
	forge_dynamic_memory_write(Forge* forge, ForgeDynamicMemory* memory, uint32_t size, uint32_t alignment, void* data);

	void
	forge_dynamic_memory_flush(Forge* forge, ForgeDynamicMemory* memory);

	void
	forge_dynamic_memory_destroy(Forge* forge, ForgeDynamicMemory* memory);
};