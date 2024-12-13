#include "Forge.h"
#include "ForgeDeletionQueue.h"
#include "ForgeUtils.h"

#include <algorithm>

namespace forge
{
	static bool
	_forge_deletion_queue_init(Forge* forge, ForgeDeletionQueue* queue)
	{
		return true;
	}

	static void
	_forge_deletion_queue_free(Forge* forge, ForgeDeletionQueue* queue)
	{
	}

	ForgeDeletionQueue*
	forge_deletion_queue_new(Forge* forge)
	{
		auto queue = new ForgeDeletionQueue();

		if (_forge_deletion_queue_init(forge, queue) == false)
		{
			forge_deletion_queue_destroy(forge, queue);
			return nullptr;
		}

		return queue;
	}

	void
	forge_deletion_queue_flush(Forge* forge, ForgeDeletionQueue* queue, bool immediate)
	{
		uint64_t value;
		auto res = vkGetSemaphoreCounterValue(forge->device, forge->timeline, &value);
		VK_RES_CHECK(res);

		auto iter = std::remove_if(queue->entries.begin(), queue->entries.end(), [forge, immediate, queue, value](const ForgeDeletionQueue::Entry& entry) {
			if (immediate || value >= entry.signal)
			{
				switch (entry.type)
				{
				case VK_OBJECT_TYPE_SWAPCHAIN_KHR:         vkDestroySwapchainKHR(forge->device, (VkSwapchainKHR)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_BUFFER:                vkDestroyBuffer(forge->device, (VkBuffer)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_IMAGE:                 vkDestroyImage(forge->device, (VkImage)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_IMAGE_VIEW:            vkDestroyImageView(forge->device, (VkImageView)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_SAMPLER:               vkDestroySampler(forge->device, (VkSampler)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_RENDER_PASS:           vkDestroyRenderPass(forge->device, (VkRenderPass)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_FRAMEBUFFER:           vkDestroyFramebuffer(forge->device, (VkFramebuffer)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_COMMAND_POOL:          vkDestroyCommandPool(forge->device, (VkCommandPool)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_SHADER_MODULE:         vkDestroyShaderModule(forge->device, (VkShaderModule)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: vkDestroyDescriptorSetLayout(forge->device, (VkDescriptorSetLayout)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_DESCRIPTOR_POOL:       vkDestroyDescriptorPool(forge->device, (VkDescriptorPool)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_PIPELINE_LAYOUT:       vkDestroyPipelineLayout(forge->device, (VkPipelineLayout)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_PIPELINE:              vkDestroyPipeline(forge->device, (VkPipeline)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_SEMAPHORE:             vkDestroySemaphore(forge->device, (VkSemaphore)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_FENCE:                 vkDestroyFence(forge->device, (VkFence)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_DEVICE_MEMORY:         vkFreeMemory(forge->device, (VkDeviceMemory)entry.handle, nullptr); break;
				case VK_OBJECT_TYPE_DEVICE:                vkDestroyDevice(forge->device, nullptr); break;
				case VK_OBJECT_TYPE_INSTANCE:              vkDestroyInstance(forge->instance, nullptr); break;
				default:
					log_error("object type '{}' is not handled", _forge_object_type_str(entry.type));
					assert(false);
					break;
				}
				return true;
			}

			return false;
		});

		queue->entries.erase(iter, queue->entries.end());
	}

	void
	forge_deletion_queue_destroy(Forge* forge, ForgeDeletionQueue* queue)
	{
		if (queue)
		{
			_forge_deletion_queue_free(forge, queue);
			delete queue;
		}
	}
};