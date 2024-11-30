#include "Forge.h"
#include "ForgeDescriptorSetManager.h"

namespace forge
{
	static bool
	_forge_descriptor_set_allocator_init(Forge* forge, ForgeShader* shader, ForgeBindingList* binding_list, ForgeDescriptorSetAllocator* allocator)
	{
		return true;
	}

	static bool
	_forge_descriptor_set_manager_free(Forge* forge, ForgeDescriptorSetManager* manager)
	{
		for (auto [layout, allocator] : manager->allocators)
			vkDestroyDescriptorPool(forge->device, allocator.pool, nullptr);
	}

	ForgeDescriptorSetManager*
	forge_descriptor_set_manager_new(Forge* forge)
	{
		auto manager = new ForgeDescriptorSetManager();

		return manager;
	}

	VkDescriptorSet
	forge_descriptor_set_manager_set_get(Forge* forge, ForgeDescriptorSetManager* manager, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		return VK_NULL_HANDLE;
	}

	void
	forge_descriptor_set_manager_flush(Forge* forge, ForgeDescriptorSetManager* manager)
	{
	
	}

	void
	forge_descriptor_set_manager_destroy(Forge* forge, ForgeDescriptorSetManager* manager)
	{
		if (manager)
		{
			_forge_descriptor_set_manager_free(forge, manager);
			delete manager;
		}
	}
};