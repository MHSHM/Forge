#include "Forge.h"
#include "ForgeDeferedQueue.h"
#include "ForgeUtils.h"

#include <algorithm>

namespace forge
{
	static void
	_forge_deferred_queue_push(Forge* forge, ForgeDeferredQueue* queue, const std::function<void()>& task, uint64_t signal)
	{
		queue->entries.push_back({task, signal});
	}

	static void
	_forge_deferred_queue_flush(Forge* forge, ForgeDeferredQueue* queue, VkSemaphore semaphore, bool immediate)
	{
		std::remove_if(queue->entries.begin(), queue->entries.end(), [forge, immediate, queue, semaphore](const ForgeDeferredQueue::Entry& entry) {
			uint64_t value;
			auto res = vkGetSemaphoreCounterValue(forge->device, semaphore, &value);
			VK_RES_CHECK(res);

			if (immediate || value >= entry.signal)
			{
				entry.task();
				return true;
			}

			return false;
		});
	}

	ForgeDeferredQueue*
	forge_deferred_queue_new(Forge* forge)
	{
		auto queue = new ForgeDeferredQueue();

		return queue;
	}

	void
	forge_deferred_queue_push(Forge* forge, ForgeDeferredQueue* queue, const std::function<void()>& task, uint64_t signal)
	{
		_forge_deferred_queue_push(forge, queue, task, signal);
	}

	void
	forge_deferred_queue_flush(Forge* forge, ForgeDeferredQueue* queue, VkSemaphore semaphore, bool immediate)
	{
		_forge_deferred_queue_flush(forge, queue, semaphore, immediate);
	}

	void
	forge_deferred_queue_destroy(Forge* forge, ForgeDeferredQueue* queue)
	{
		if (queue)
		{
			delete queue;
		}
	}
};