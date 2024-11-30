#pragma once

#include "ForgeShader.h"

#include <utility>

namespace forge
{
	struct Forge;
	struct ForgeBuffer;
	struct ForgeImage;

	static constexpr uint32_t FORGE_MAX_VERTEX_BUFFER_BINDINGS = 16u;
	static constexpr uint32_t FORGE_MAX_IMAGE_BINDINGS = 16u;

	struct ForgeBindingList
	{
		std::pair<uint32_t, void*> uniforms[FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS];
		ForgeBuffer* vertex_buffers[FORGE_MAX_VERTEX_BUFFER_BINDINGS];
		ForgeBuffer* index_buffer;
		ForgeImage* images[FORGE_MAX_IMAGE_BINDINGS];
	};

	bool
	forge_binding_list_uniform_write(Forge* forge, ForgeBindingList* list, ForgeShader* shader, uint32_t binding, const std::pair<uint32_t, void*>& block);

	bool
	forge_binding_list_vertex_buffer_bind(Forge* forge, ForgeBindingList* list, ForgeShader* shader, uint32_t binding, ForgeBuffer* buffer);

	bool
	forge_binding_list_index_buffer_bind(Forge* forge, ForgeBindingList* list, ForgeShader* shader, ForgeBuffer* buffer);

	bool
	forge_binding_list_image_bind(Forge* forge, ForgeBindingList* list, ForgeShader* shader, uint32_t binding, ForgeImage* image);
};