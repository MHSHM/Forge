#include "Forge.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeImage.h"
#include "ForgeShader.h"
#include "ForgeLogger.h"
#include "ForgeBindingList.h"
#include "ForgeUtils.h"

namespace forge
{
	static bool
	_forge_descriptor_set_allocator_init(Forge* forge, ForgeShader* shader, ForgeDescriptorSetAllocator* allocator)
	{
		uint32_t uniforms_count = 0u;
		for (auto uniform : shader->description.uniforms)
		{
			if (uniform.name.empty() == false)
			{
				++uniforms_count;
			}
		}

		std::vector<VkDescriptorPoolSize> pool_sizes;
		if (uniforms_count > 0u)
		{
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, uniforms_count * FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS});
		}


		VkDescriptorPoolCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS;
		info.poolSizeCount = (uint32_t)pool_sizes.size();
		info.pPoolSizes = pool_sizes.data();
		auto res = vkCreateDescriptorPool(forge->device, &info, nullptr, &allocator->pool);
		VK_RES_CHECK(res);

		if (res != VK_SUCCESS)
		{
			log_error("Failed to initialize descriptor set allocator");
			return false;
		}

		return true;
	}

	static VkDescriptorSet
	_forge_descriptor_set_allocator_set_get(Forge* forge, ForgeDescriptorSetAllocator* allocator, ForgeShader* shader, ForgeBindingList* binding_list)
	{
		uint64_t hash = 0;

		// Generate hash for bindings that changing them could invoke a vkDescriptorSetUpdate

		for (auto image : binding_list->images)
		{
			if (image == nullptr)
				continue;

			_forge_hash_combine(hash, image);
			_forge_hash_combine(hash, image->handle);
			_forge_hash_combine(hash, image->description.name);
		}

		if (auto iter = allocator->recently_used.find(hash); iter != allocator->recently_used.end())
		{
			iter->second.last_use = 0u; // TODO: set to the current frame index
			return iter->second.handle;
		}
		else
		{
			VkDescriptorSet set{};

			if (allocator->available.size() > 0)
			{
				set = allocator->available.back();
				allocator->available.pop_back();

				return set;
			}
			else
			{
				assert(allocator->allocatoed_sets < FORGE_DESCRIPTOR_SET_ALLOCATOR_MAX_SETS);

				VkDescriptorSetAllocateInfo allocate_info {};
				allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocate_info.descriptorSetCount = 1u;
				allocate_info.pSetLayouts = &shader->descriptor_set_layout;
				allocate_info.descriptorPool = allocator->pool;
				auto res = vkAllocateDescriptorSets(forge->device, &allocate_info, &set);
				VK_RES_CHECK(res);

				++allocator->allocatoed_sets;
			}

			allocator->recently_used[hash] = {set, 0u}; // TODO: set to the current frame index
			// TODO: Update descriptor set

			return set;
		}
	}

	static void
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
		if (auto iter = manager->allocators.find(shader->descriptor_set_layout); iter != manager->allocators.end())
		{
			return _forge_descriptor_set_allocator_set_get(forge, &iter->second, shader, binding_list);
		}
		else
		{
			manager->allocators[shader->descriptor_set_layout] = {};
			_forge_descriptor_set_allocator_init(forge, shader, &manager->allocators[shader->descriptor_set_layout]);
			return _forge_descriptor_set_allocator_set_get(forge, &manager->allocators[shader->descriptor_set_layout], shader, binding_list);
		}
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