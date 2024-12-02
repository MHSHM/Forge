#include "Forge.h"
#include "ForgeDynamicMemory.h"

namespace forge
{
	ForgeDynamicMemory*
	forge_dynamic_memory_new(Forge* forge, uint32_t size, VkBufferUsageFlags usage)
	{
		return nullptr;
	}

	void
	forge_dynamic_memory_write(Forge* forge, ForgeDynamicMemory* memory, uint32_t size, uint32_t alignment, void* data)
	{
	
	}

	void
	forge_dynamic_memory_flush(Forge* forge, ForgeDynamicMemory* memory)
	{
	
	}

	void
	forge_dynamic_memory_destroy(Forge* forge, ForgeDynamicMemory* memory)
	{
	
	}
};