#include <iostream>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Forge.h>
#include <ForgeLogger.h>
#include <ForgeSwapchain.h>
#include <ForgeBuffer.h>
#include <ForgeImage.h>
#include <ForgeRenderPass.h>
#include <ForgeDeletionQueue.h>
#include <ForgeShader.h>
#include <ForgeBindingList.h>
#include <ForgeDescriptorSetManager.h>
#include <ForgeFrame.h>

#include <sstream>
#include <fstream>

inline static void
_glfw_error_callback(int error, const char* description)
{
	forge::log_error("GLFW is reporting an error with code '{}' and message '{}'");
}

inline static std::string
_shader_code_read(const char* path)
{
	std::stringstream source_code;
	std::ifstream shader_file(path);
	std::string line;

	if (!shader_file.is_open())
	{
		forge::log_error("Failed to load the shader with the given path '{}'\n", path);
		return "";
	}

	while (std::getline(shader_file, line))
	{
		source_code << line << '\n';
	}

	shader_file.close();
	return source_code.str();
}

int main()
{
	if (!glfwInit())
		return -1;
	glfwSetErrorCallback(_glfw_error_callback);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Window Title", NULL, NULL);
	glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
	auto hwnd = glfwGetWin32Window(window);

	auto forge = forge::forge_new();

	forge::ForgeSwapchainDescription desc {};
	desc.extent = {800, 600};
	desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	desc.images_count = 2u;
	desc.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	desc.window = hwnd;
	auto swapchain_frame = forge::forge_frame_new(forge, desc);

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f
	};

	forge::ForgeBufferDescription vertex_buffer_desc {};
	vertex_buffer_desc.name = "Vertex buffer";
	vertex_buffer_desc.size = sizeof(vertices);
	vertex_buffer_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	vertex_buffer_desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	auto vertex_buffer = forge::forge_buffer_new(forge, vertex_buffer_desc);
	forge_buffer_write(forge, vertex_buffer, vertices, sizeof(vertices));

	forge::ForgePipelineDescription pipeline_desc {};
	pipeline_desc.bindings[0].binding = 0u;
	pipeline_desc.bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	pipeline_desc.bindings[0].stride = 3 * sizeof(float);
	pipeline_desc.blend_desc[0].blendEnable = true;
	pipeline_desc.blend_desc[0].colorBlendOp = VK_BLEND_OP_ADD;
	pipeline_desc.blend_desc[0].alphaBlendOp = VK_BLEND_OP_ADD;
	pipeline_desc.blend_desc[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	pipeline_desc.blend_desc[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_desc.blend_desc[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	pipeline_desc.blend_desc[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipeline_desc.blend_desc[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
												 VK_COLOR_COMPONENT_G_BIT |
												 VK_COLOR_COMPONENT_B_BIT |
												 VK_COLOR_COMPONENT_A_BIT;

	auto shader_code = _shader_code_read("shader.glsl");
	auto shader = forge::forge_shader_new(forge, pipeline_desc, "Shader", shader_code.c_str());

	float model_mat[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	while (!glfwWindowShouldClose(window))
	{
		forge::ForgeBindingList binding_list {};
		forge::forge_binding_list_vertex_buffer_bind(forge, &binding_list, shader, 0u, vertex_buffer);
		forge::forge_binding_list_uniform_write(forge, &binding_list, shader, 0u, {sizeof(model_mat), model_mat});

		forge::forge_frame_prepare(forge, swapchain_frame, shader, &binding_list);
		forge::forge_frame_begin(forge, swapchain_frame);
		forge::forge_frame_bind_resources(forge, swapchain_frame, shader, &binding_list);
		forge::forge_frame_draw(forge, swapchain_frame, 6u);
		forge::forge_frame_end(forge, swapchain_frame);
		forge::forge_flush(forge);

		glfwPollEvents();
	}

	forge::forge_buffer_destroy(forge, vertex_buffer);
	forge::forge_shader_destroy(forge, shader);
	forge::forge_frame_destroy(forge, swapchain_frame);
	forge::forge_destroy(forge);

	return 0;
}