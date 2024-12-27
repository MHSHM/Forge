#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>

namespace forge
{
	struct Forge;
	struct ForgeBindingList;
	struct ForgeShader;

	static constexpr uint32_t FORGE_DESCRIPTOR_SET_MANAGER_SET_MAX_AGE = 10u; // frames
	static constexpr uint32_t FORGE_DESCRIPTOR_SET_MANAGER_MAX_DESCRIPTOR_SETS = 256u;
	static constexpr uint32_t FORGE_DESCRIPTOR_SET_MANAGER_MAX_SAMPLED_IMAGES = FORGE_DESCRIPTOR_SET_MANAGER_MAX_DESCRIPTOR_SETS * 64u;
	static constexpr uint32_t FORGE_DESCRIPTOR_SET_MANAGER_MAX_STORAGE_IMAGES = FORGE_DESCRIPTOR_SET_MANAGER_MAX_DESCRIPTOR_SETS * 64u;
	static constexpr uint32_t FORGE_DESCRIPTOR_SET_MANAGER_MAX_DYNAMIC_UNIFORM_BUFFERS = FORGE_DESCRIPTOR_SET_MANAGER_MAX_DESCRIPTOR_SETS * 16u;

	struct ForgeDescriptorSet
	{
		VkDescriptorSet handle;
		VkDescriptorSetLayout layout;
		uint64_t active_bindings_hash;
		uint64_t release_signal;
	};

	struct ForgeDescriptorSetManager
	{
		VkDescriptorPool pool;
		std::vector<ForgeDescriptorSet> allocated_sets;
	};

	ForgeDescriptorSetManager*
	forge_descriptor_set_manager_new(Forge* forge);

	VkDescriptorSet
	forge_descriptor_set_acquire(Forge* forge, ForgeDescriptorSetManager* manager, ForgeShader* shader, ForgeBindingList* binding_list);

	void
	forge_descriptor_set_manager_destroy(Forge* forge, ForgeDescriptorSetManager* manager);
};