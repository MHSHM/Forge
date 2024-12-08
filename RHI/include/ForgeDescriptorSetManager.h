#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>

namespace forge
{
	struct Forge;
	struct ForgeBindingList;
	struct ForgeShader;

	static constexpr uint32_t FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS = 16u;
	static constexpr uint32_t FORGE_DESCRIPTOR_SET_ALLOCATOR_SET_MAX_AGE = 10u; // frames

	struct ForgeDescriptorSet
	{
		VkDescriptorSet handle;
		uint64_t last_use; // frame index
	};

	struct ForgeDescriptorSetAllocator
	{
		VkDescriptorPool pool;
		std::unordered_map<uint64_t, ForgeDescriptorSet> recently_used;
		std::vector<VkDescriptorSet> available;
		uint32_t allocatoed_sets;
	};

	struct ForgeDescriptorSetManager
	{
		std::unordered_map<VkDescriptorSetLayout, ForgeDescriptorSetAllocator> allocators;
		uint32_t allocated_descriptor_sets;
	};

	ForgeDescriptorSetManager*
	forge_descriptor_set_manager_new(Forge* forge);

	VkDescriptorSet
	forge_descriptor_set_acquire(Forge* forge, ForgeDescriptorSetManager* manager, ForgeShader* shader, ForgeBindingList* binding_list);

	void
	forge_descriptor_set_manager_flush(Forge* forge, ForgeDescriptorSetManager* manager);

	void
	forge_descriptor_set_manager_destroy(Forge* forge, ForgeDescriptorSetManager* manager);
};