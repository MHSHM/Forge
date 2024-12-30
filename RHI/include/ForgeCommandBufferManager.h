#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace forge
{
	struct Forge;

	struct ForgeCommandBufferManager
	{
		std::vector<VkCommandBuffer> available_command_buffers;
		VkCommandPool pool;
	};

	ForgeCommandBufferManager*
	forge_command_buffer_manager_new(Forge* forge);

	void
	forge_command_buffer_manager_destroy(Forge* forge, ForgeCommandBufferManager* manager);

	VkCommandBuffer
	forge_command_buffer_acquire(Forge* forge, ForgeCommandBufferManager* manager, bool begin);
}