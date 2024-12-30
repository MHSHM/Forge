#include "Forge.h"
#include "ForgeDeferredQueue.h"
#include "ForgeLogger.h"
#include "ForgeUtils.h"
#include "ForgeCommandBufferManager.h"

namespace forge
{
	static void
	_forge_object_destroy(Forge* forge, void* handle, void* pool, VkObjectType type)
	{
		switch (type)
		{
		case VK_OBJECT_TYPE_SWAPCHAIN_KHR:         vkDestroySwapchainKHR(forge->device, (VkSwapchainKHR)handle, nullptr); break;
		case VK_OBJECT_TYPE_BUFFER:                vkDestroyBuffer(forge->device, (VkBuffer)handle, nullptr); break;
		case VK_OBJECT_TYPE_IMAGE:                 vkDestroyImage(forge->device, (VkImage)handle, nullptr); break;
		case VK_OBJECT_TYPE_IMAGE_VIEW:            vkDestroyImageView(forge->device, (VkImageView)handle, nullptr); break;
		case VK_OBJECT_TYPE_SAMPLER:               vkDestroySampler(forge->device, (VkSampler)handle, nullptr); break;
		case VK_OBJECT_TYPE_RENDER_PASS:           vkDestroyRenderPass(forge->device, (VkRenderPass)handle, nullptr); break;
		case VK_OBJECT_TYPE_FRAMEBUFFER:           vkDestroyFramebuffer(forge->device, (VkFramebuffer)handle, nullptr); break;
		case VK_OBJECT_TYPE_COMMAND_POOL:          vkDestroyCommandPool(forge->device, (VkCommandPool)handle, nullptr); break;
		case VK_OBJECT_TYPE_SHADER_MODULE:         vkDestroyShaderModule(forge->device, (VkShaderModule)handle, nullptr); break;
		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: vkDestroyDescriptorSetLayout(forge->device, (VkDescriptorSetLayout)handle, nullptr); break;
		case VK_OBJECT_TYPE_DESCRIPTOR_POOL:       vkDestroyDescriptorPool(forge->device, (VkDescriptorPool)handle, nullptr); break;
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT:       vkDestroyPipelineLayout(forge->device, (VkPipelineLayout)handle, nullptr); break;
		case VK_OBJECT_TYPE_PIPELINE:              vkDestroyPipeline(forge->device, (VkPipeline)handle, nullptr); break;
		case VK_OBJECT_TYPE_SEMAPHORE:             vkDestroySemaphore(forge->device, (VkSemaphore)handle, nullptr); break;
		case VK_OBJECT_TYPE_FENCE:                 vkDestroyFence(forge->device, (VkFence)handle, nullptr); break;
		case VK_OBJECT_TYPE_DEVICE_MEMORY:         vkFreeMemory(forge->device, (VkDeviceMemory)handle, nullptr); break;
		case VK_OBJECT_TYPE_DEVICE:                vkDestroyDevice(forge->device, nullptr); break;
		case VK_OBJECT_TYPE_INSTANCE:              vkDestroyInstance(forge->instance, nullptr); break;
		default:
			log_error("object type '{}' is not handled", _forge_object_type_str(type));
			assert(false);
			break;
		}
	}

	static void
	_forge_command_buffer_recycle(Forge* forge, VkCommandBuffer command_buffer)
	{
		auto& command_buffer_manager = forge->command_buffer_manager;

		command_buffer_manager->available_command_buffers.push_back(command_buffer);
	}

	ForgeDeferredQueue*
	forge_deferred_queue_new(Forge* forge)
	{
		auto queue = new ForgeDeferredQueue();
		return queue;
	}

	void
	forge_deferred_queue_destroy(Forge* forge, ForgeDeferredQueue* queue)
	{
		if (queue)
		{
			delete queue;
		}
	}

	void
	forge_deferred_command_buffer_recycle(Forge* forge, ForgeDeferredQueue* queue, VkCommandBuffer command_buffer)
	{
		ForgeDeferredQueue::DeferredTask task {};
		task.type = ForgeDeferredQueue::RECYCLE_COMMAND_BUFFER;
		task.execution_signal = forge->timeline_next_check_point;
		task.command_buffer_recycle.command_buffer = command_buffer;
		queue->deferred_tasks.push_back(std::move(task));
	}

	void
	forge_deferred_queue_flush(Forge* forge, ForgeDeferredQueue* queue, bool immediate)
	{
		uint64_t value;
		auto res = vkGetSemaphoreCounterValue(forge->device, forge->timeline, &value);
		VK_RES_CHECK(res);

		auto iter = std::remove_if(queue->deferred_tasks.begin(), queue->deferred_tasks.end(), [forge, immediate, queue, value](const ForgeDeferredQueue::DeferredTask& task) {
			if (immediate || value >= task.execution_signal)
			{
				switch (task.type)
				{
				case ForgeDeferredQueue::DESTROY_OBJECT:         _forge_object_destroy(forge, task.object_destroy.handle, task.object_destroy.pool, task.object_destroy.type); break;
				case ForgeDeferredQueue::RECYCLE_COMMAND_BUFFER: _forge_command_buffer_recycle(forge, task.command_buffer_recycle.command_buffer); break;
				default:
					assert(false);
					log_error("Unhandled deferred task type");
					break;
				}

				return true;
			}

			return false;
		});

		queue->deferred_tasks.erase(iter, queue->deferred_tasks.end());
	}
};