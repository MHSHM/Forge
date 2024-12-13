#pragma once

#include "Forge.h"
#include "ForgeUtils.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace forge
{
	struct ForgeDeletionQueue
	{
		struct Entry
		{
			uint64_t signal;
			VkObjectType type;
			void* handle;
		};

		std::vector<Entry> entries;
	};

	ForgeDeletionQueue*
	forge_deletion_queue_new(Forge* forge);

	template<typename T> void
	forge_deletion_queue_push(Forge* forge, ForgeDeletionQueue* queue, T handle)
	{
		ForgeDeletionQueue::Entry entry{};
		entry.signal = forge->timeline_next_check_point;
		entry.handle = handle;
		entry.type = _vk_object_type<T>();
		queue->entries.push_back(std::move(entry));
	};

	void
	forge_deletion_queue_flush(Forge* forge, ForgeDeletionQueue* queue, bool immediate);

	void
	forge_deletion_queue_destroy(Forge* forge, ForgeDeletionQueue* queue);
};