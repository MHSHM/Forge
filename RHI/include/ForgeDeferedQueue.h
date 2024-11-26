#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

namespace forge
{
	struct Forge;

	struct ForgeDeferedQueue
	{
		struct Entry
		{
			std::function<void()> task;
			uint64_t signal;
		};

		VkSemaphore semaphore;
		std::vector<Entry> entries;
	};

	ForgeDeferedQueue*
	forge_defered_queue_new(Forge* forge);

	void
	forge_defered_queue_flush(Forge* forge, ForgeDeferedQueue* queue);

	void
	forge_defered_queue_destroy(Forge* forge, ForgeDeferedQueue* queue);
};