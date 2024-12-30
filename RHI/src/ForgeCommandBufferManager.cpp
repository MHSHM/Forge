#include "Forge.h"
#include "ForgeCommandBufferManager.h"
#include "ForgeLogger.h"
#include "ForgeDeletionQueue.h"

#include <algorithm>

namespace forge
{
	static void
	_forge_command_buffer_begin(ForgeCommandBuffer command_buffer)
	{
		VkCommandBufferBeginInfo begin_info {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		auto res = vkBeginCommandBuffer(command_buffer.handle, &begin_info);
		VK_RES_CHECK(res);
	}

	static bool
	_forge_command_buffer_manager_init(Forge* forge, ForgeCommandBufferManager* manager)
	{
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = forge->queue_family_index;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		auto res = vkCreateCommandPool(forge->device, &info, nullptr, &manager->pool);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to create the global command pool");
			return false;
		}

		return true;
	}

	static void
	_forge_command_buffer_manager_free(Forge* forge, ForgeCommandBufferManager* manager)
	{
		if (manager->pool)
		{
			forge_deletion_queue_push(forge, forge->deletion_queue, manager->pool);
		}
	}

	VkCommandBuffer
	forge_command_buffer_acquire(Forge* forge, ForgeCommandBufferManager* manager, bool begin)
	{
		uint64_t value;
		auto res = vkGetSemaphoreCounterValue(forge->device, forge->timeline, &value);
		VK_RES_CHECK(res);

		for (auto& command_buffer : manager->allocated_cb)
		{
			if (value >= command_buffer.release_signal)
			{
				if (begin)
				{
					_forge_command_buffer_begin(command_buffer);
				}
				command_buffer.release_signal = forge->timeline_next_signal;

				return command_buffer.handle;
			}
		}

		ForgeCommandBuffer command_buffer {};
		command_buffer.release_signal = forge->timeline_next_signal;
		command_buffer.pool = manager->pool;

		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandBufferCount = 1u;
		alloc_info.commandPool = manager->pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		res = vkAllocateCommandBuffers(forge->device, &alloc_info, &command_buffer.handle);
		VK_RES_CHECK(res);

		if (begin)
		{
			_forge_command_buffer_begin(command_buffer);
		}

		manager->allocated_cb.push_back(command_buffer);

		return command_buffer.handle;
	}

	ForgeCommandBufferManager*
	forge_command_buffer_manager_new(Forge* forge)
	{
		auto manager = new ForgeCommandBufferManager();

		if (_forge_command_buffer_manager_init(forge, manager) == false)
		{
			forge_command_buffer_manager_destroy(forge, manager);
			return nullptr;
		}

		return manager;
	}

	void
	forge_command_buffer_manager_destroy(Forge* forge, ForgeCommandBufferManager* manager)
	{
		if (manager)
		{
			_forge_command_buffer_manager_free(forge, manager);
			delete manager;
		}
	}
};