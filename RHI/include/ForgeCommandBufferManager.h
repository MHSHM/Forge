#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace forge
{
	struct Forge;

	struct ForgeCommandBuffer
	{
		VkCommandBuffer handle;
		VkCommandPool pool;
		uint64_t release_signal;
	};

	struct ForgeCommandBufferManager
	{
		VkCommandPool pool;
		std::vector<ForgeCommandBuffer> allocated_cb;
	};

	ForgeCommandBufferManager*
	forge_command_buffer_manager_new(Forge* forge);

	void
	forge_command_buffer_manager_destroy(Forge* forge, ForgeCommandBufferManager* manager);

	VkCommandBuffer
	forge_command_buffer_acquire(Forge* forge, ForgeCommandBufferManager* manager, bool begin);

}