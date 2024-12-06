#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace forge
{
	struct Forge;

	struct ForgeCommandBufferManager
	{
		struct ForgeCommandBuffer
		{
			VkCommandBuffer handle;
			uint64_t release_signal;
		};

		std::vector<VkCommandBuffer> available;
		std::vector<ForgeCommandBuffer> to_be_released;
		VkCommandPool pool;
	};

	ForgeCommandBufferManager*
	forge_command_buffer_manager_new(Forge* forge);

	void
	forge_command_buffer_manager_destroy(Forge* forge, ForgeCommandBufferManager* manager);

	VkCommandBuffer
	forge_command_buffer_acquire(Forge* forge, ForgeCommandBufferManager* manager);

	void
	forge_command_buffer_release(Forge* forge, ForgeCommandBufferManager* manager, VkCommandBuffer command_buffer);

	void
	forge_command_buffer_manager_flush(Forge* forge, ForgeCommandBufferManager* manager);
}