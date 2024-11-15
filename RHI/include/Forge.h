#pragma once

#include <vulkan/vulkan.h>

namespace forge
{
	struct Forge
	{
		VkInstance instance;
	};

	Forge* forge_new();

	void forge_destroy(Forge* forge);
};