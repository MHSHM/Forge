#include "Forge.h"
#include "ForgeRenderPass.h"

namespace forge
{
	static bool
	_forge_render_pass_init(Forge* forge, ForgeRenderPass* render_pass)
	{
		return true;
	}

	static void
	_forge_render_pass_free(Forge* forge, ForgeRenderPass* render_pass)
	{
		if (render_pass->framebuffer)
		{
			vkDestroyFramebuffer(forge->device, render_pass->framebuffer, nullptr);
		}

		if (render_pass->handle)
		{
			vkDestroyRenderPass(forge->device, render_pass->handle, nullptr);
		}
	}

	ForgeRenderPass*
	forge_render_pass_new(Forge* forge, ForgeRenderPassDescription description)
	{
		auto render_pass = new ForgeRenderPass();
		render_pass->description = description;

		if (_forge_render_pass_init(forge, render_pass) == false)
		{
			forge_render_pass_destroy(forge, render_pass);
			return nullptr;
		}

		return render_pass;
	}

	void
	forge_render_pass_destroy(Forge* forge, ForgeRenderPass* render_pass)
	{
		if (render_pass)
		{
			_forge_render_pass_free(forge, render_pass);
			delete render_pass;
		}
	}
};