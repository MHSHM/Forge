#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <vector>

namespace forge
{
	struct Forge;

	struct ForgeDeferredQueue
	{
		struct Entry
		{
			std::function<void()> task;
			uint64_t signal;
		};

		VkSemaphore semaphore;
		std::vector<Entry> entries;
	};

	ForgeDeferredQueue*
	forge_deferred_queue_new(Forge* forge);

	void
	forge_deferred_queue_push(Forge* forge, ForgeDeferredQueue* queue, const std::function<void()>& task, uint64_t signal);

	void
	forge_deferred_queue_flush(Forge* forge, ForgeDeferredQueue* queue, bool immediate);

	void
	forge_deferred_queue_destroy(Forge* forge, ForgeDeferredQueue* queue);
};