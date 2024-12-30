#include "Forge.h"
#include "ForgeCommandBufferManager.h"
#include "ForgeLogger.h"
#include "ForgeDeferredQueue.h"

#include <algorithm>

namespace forge
{
	static void
	_forge_command_buffer_begin(VkCommandBuffer command_buffer)
	{
		VkCommandBufferBeginInfo begin_info {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		auto res = vkBeginCommandBuffer(command_buffer, &begin_info);
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
			forge_deferred_object_destroy(forge, forge->deferred_queue, manager->pool);
		}
	}

	VkCommandBuffer
	forge_command_buffer_acquire(Forge* forge, ForgeCommandBufferManager* manager, bool begin)
	{
		VkCommandBuffer command_buffer = VK_NULL_HANDLE;

		if (manager->available_command_buffers.size() > 0)
		{
			command_buffer = manager->available_command_buffers.back();
			manager->available_command_buffers.pop_back();
		}
		else
		{
			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandBufferCount = 1u;
			alloc_info.commandPool = manager->pool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			auto res = vkAllocateCommandBuffers(forge->device, &alloc_info, &command_buffer);
			VK_RES_CHECK(res);
		}

		if (begin)
		{
			_forge_command_buffer_begin(command_buffer);
		}

		// Schedule a command buffer recycle task
		forge_deferred_command_buffer_recycle(forge, forge->deferred_queue, command_buffer);

		return command_buffer;
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