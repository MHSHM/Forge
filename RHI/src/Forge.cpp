#include "Forge.h"

#include <vulkan/vulkan.h>

namespace forge
{
	static bool _forge_init(Forge* forge)
	{
		if (forge == nullptr)
		{
			return false;
		}

		return true;
	}

	static void _forge_free(Forge* forge)
	{

	}

	Forge* forge_new()
	{
		auto forge = new Forge;
		bool init = _forge_init(forge);
		if (init == false)
		{
			forge_destroy(forge);
			return  nullptr;
		}

		return forge;
	}

	void forge_destroy(Forge* forge)
	{
		if (forge)
		{
			_forge_free(forge);
			delete forge;
		}
	}
};