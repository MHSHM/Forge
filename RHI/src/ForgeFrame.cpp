#include "Forge.h"
#include "ForgeFrame.h"
#include "ForgeSwapchain.h"
#include "ForgeRenderPass.h"
#include "ForgeDeferedQueue.h"
#include "ForgeDescriptorSetManager.h"
#include "ForgeDynamicMemory.h"

namespace forge
{
	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeImage* color, ForgeImage* depth)
	{
		return nullptr;
	}

	ForgeFrame*
	forge_frame_new(Forge* forge, ForgeSwapchainDescription swapchain_desc)
	{
		return nullptr;
	}

	bool
	forge_frame_begin(Forge* forge, ForgeFrame* frame)
	{
		return true;
	}

	void
	forge_frame_end(Forge* forge, ForgeFrame* frame)
	{
	
	}

	void
	forge_frame_destroy(Forge* forge, ForgeFrame* frame)
	{
	
	}
};