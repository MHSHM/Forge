#pragma once

#include "ForgeUtils.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace forge
{
	struct Forge;

	struct ForgeDeferredQueue
	{
		enum DeferredTaskType
		{
			DESTROY_OBJECT,
			RECYCLE_COMMAND_BUFFER,
			RECYCLE_DESCRIPTOR_SET
		};

		struct DeferredTask
		{
			DeferredTaskType type;
			uint64_t execution_signal;

			union
			{
				struct
				{
					void* handle;
					void* pool;
					VkObjectType type;
				} object_destroy;

				struct
				{
					VkCommandBuffer command_buffer;
				} command_buffer_recycle;
			};
		};

		std::vector<DeferredTask> deferred_tasks;
	};

	ForgeDeferredQueue*
	forge_deferred_queue_new(Forge* forge);

	void
	forge_deferred_queue_destroy(Forge* forge, ForgeDeferredQueue* queue);

	template<typename T> void
	forge_deferred_object_destroy(Forge* forge, ForgeDeferredQueue* queue, T handle, void* pool = nullptr)
	{
		ForgeDeferredQueue::DeferredTask task {};
		task.type = ForgeDeferredQueue::DESTROY_OBJECT;
		task.execution_signal = forge->timeline_next_check_point;
		task.object_destroy.handle = handle;
		task.object_destroy.pool = pool;
		task.object_destroy.type = _vk_object_type<T>();
		queue->deferred_tasks.push_back(std::move(task));
	};

	void
	forge_deferred_command_buffer_recycle(Forge* forge, ForgeDeferredQueue* queue, VkCommandBuffer command_buffer);

	void
	forge_deferred_queue_flush(Forge* forge, ForgeDeferredQueue* queue, bool immediate);
};