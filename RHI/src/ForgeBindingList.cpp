#include "Forge.h"
#include "ForgeBindingList.h"
#include "ForgeBuffer.h"
#include "ForgeImage.h"
#include "ForgeLogger.h"

#include <assert.h>

namespace forge
{
	bool
	forge_binding_list_uniform_write(Forge* forge, ForgeBindingList* list, ForgeShader* shader, uint32_t binding, const std::pair<uint32_t, void*>& block)
	{
		if (binding >= FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS)
		{
			log_error("The provided binding '{}' exceeds the binding limit of uniform buffers '{}'", binding, FORGE_SHADER_MAX_DYNAMIC_UNIFORM_BUFFERS);
			return false;
		}

		auto& uniform_layout = shader->description.uniforms[binding];
		if (uniform_layout.size != block.first)
		{
			log_error("The provided size '{}' doesn't match the uniform block's size '{}'", block.first, uniform_layout.size);
			return false;
		}

		list->uniforms[binding] = block;

		return true;
	}

	bool
	forge_binding_list_vertex_buffer_bind(Forge* forge, ForgeBindingList* list, ForgeShader* shader, uint32_t binding, ForgeBuffer* buffer)
	{
		if (binding >= FORGE_MAX_VERTEX_BUFFER_BINDINGS)
		{
			log_error("The provided binding '{}' exceeds the binding limit of vertex buffers '{}'", binding, FORGE_MAX_VERTEX_BUFFER_BINDINGS);
			return false;
		}

		auto& attribute_layout = shader->description.attributes[binding];
		if (attribute_layout.name.empty() == true)
		{
			log_error("The provided binding '{}' doesn't map to a valid binding in the shader '{}'", binding, shader->description.name);
			return false;
		}

		auto& usage = buffer->description.usage;
		if ((usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == 0)
		{
			log_error("The provided buffer is not a vertex buffer");
			return false;
		}

		list->vertex_buffers[binding] = buffer;

		return true;
	}

	bool
	forge_binding_list_index_buffer_bind(Forge* forge, ForgeBindingList* list, ForgeShader* shader, ForgeBuffer* buffer)
	{
		auto& usage = buffer->description.usage;
		if ((usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) == 0)
		{
			log_error("The provided buffer is not an index buffer");
			return false;
		}

		list->index_buffer = buffer;

		return true;
	}

	bool
	forge_binding_list_image_bind(Forge* forge, ForgeBindingList* list, ForgeShader* shader, uint32_t binding, ForgeImage* image)
	{
		if (binding >= FORGE_MAX_IMAGE_BINDINGS)
		{
			log_error("The provided binding '{}' doesn't map to a valid binding in the shader '{}'", binding, shader->description.name);
			return false;
		}

		auto& image_layout = shader->description.images[binding];
		if (image_layout.type != image->view_type)
		{
			log_error("The provided image '{}' doesn't have an image view that matches the one defined in the shader", image->description.name, shader->description.name);
			return false;
		}

		if (image_layout.storage == true)
		{
			if ((image->description.usage & VK_IMAGE_USAGE_STORAGE_BIT) == 0)
			{
				log_error("The privded image '{}' is not marked as a storage image while the shader '{}' expects a storage image", image->description.name, shader->description.name);
				return false;
			}
		}
		else
		{
			if ((image->description.usage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0)
			{
				log_error("The provided image '{}' is not marked as a sampled image while the shader '{}' expects a sampled image", image->description.name, shader->description.name);
				return false;
			}
		}

		list->images[binding] = image;

		return true;
	}
};