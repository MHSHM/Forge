#include "Forge.h"
#include "ForgeDeferedQueue.h"
#include "ForgeUtils.h"

#include <algorithm>

namespace forge
{
	static bool
	_forge_deferred_queue_init(Forge* forge, ForgeDeferredQueue* queue)
	{
		VkSemaphoreTypeCreateInfo timeline_info {};
		timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timeline_info.initialValue = 0;

		VkSemaphoreCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		create_info.pNext = &timeline_info;
		create_info.flags = 0;

		auto res = vkCreateSemaphore(forge->device, &create_info, NULL, &queue->semaphore);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create the deferred queue, the following error '{}' is reported", _forge_result_to_str(res));
			return false;
		}

		log_info("Deferred queue was created successfully");

		return true;
	}

	static void
	_forge_deferred_queue_push(Forge* forge, ForgeDeferredQueue* queue, const std::function<void()>& task, uint64_t signal)
	{
		queue->entries.push_back({task, signal});
	}

	static void
	_forge_deferred_queue_flush(Forge* forge, bool immediate, ForgeDeferredQueue* queue)
	{
		std::remove_if(queue->entries.begin(), queue->entries.end(), [forge, immediate, queue](const ForgeDeferredQueue::Entry& entry) {
			uint64_t value;
			auto res = vkGetSemaphoreCounterValue(forge->device, queue->semaphore, &value);
			VK_RES_CHECK(res);

			if (immediate || value >= entry.signal)
			{
				entry.task();
				return true;
			}

			return false;
		});
	}

	static void
	_forge_deferred_queue_free(Forge* forge, ForgeDeferredQueue* queue)
	{
		if (queue->semaphore)
		{
			vkDestroySemaphore(forge->device, queue->semaphore, nullptr);
		}
	}

	ForgeDeferredQueue*
	forge_deferred_queue_new(Forge* forge)
	{
		auto queue = new ForgeDeferredQueue();

		if (_forge_deferred_queue_init(forge, queue) == false)
		{
			return nullptr;
		}

		return queue;
	}

	void
	forge_deferred_queue_push(Forge* forge, ForgeDeferredQueue* queue, const std::function<void()>& task, uint64_t signal)
	{
		_forge_deferred_queue_push(forge, queue, task, signal);
	}

	void
	forge_deferred_queue_flush(Forge* forge, ForgeDeferredQueue* queue, bool immediate)
	{
		_forge_deferred_queue_flush(forge, immediate, queue);
	}

	void
	forge_deferred_queue_destroy(Forge* forge, ForgeDeferredQueue* queue)
	{
		if (queue)
		{
			_forge_deferred_queue_free(forge, queue);
			delete queue;
		}
	}
};