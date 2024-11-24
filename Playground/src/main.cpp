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

inline static void
_glfw_error_callback(int error, const char* description)
{
	forge::log_error("GLFW is reporting an error with code '{}' and message '{}'");
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
	auto swapchain = forge::forge_swapchain_new(forge, desc);

    float vertices[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

	forge::ForgeBufferDescription buffer_desc{};
	buffer_desc.name = "Forge buffer";
	buffer_desc.size = 1024;
	buffer_desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    auto buffer = forge::forge_buffer_new(forge, buffer_desc);

    forge::forge_buffer_write(forge, buffer, vertices, sizeof(vertices));

    std::vector<float> data;
    for (uint32_t i = 0; i < 1024 * 1024 * 4; ++i)
    {
        data.push_back(1.0f);
    }

    forge::ForgeImageDescription image_desc {};
    image_desc.extent = {1024, 1024, 1};
    image_desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    image_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    image_desc.name = "Forge Image";
    image_desc.type = VK_IMAGE_TYPE_2D;
    image_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_desc.mipmaps = true;
    auto image = forge::forge_image_new(forge, image_desc);

    forge::ForgeRenderPassDescription render_pass_desc {};
    render_pass_desc.colors[0].image = image;
    render_pass_desc.colors[0].initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    render_pass_desc.colors[0].final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    render_pass_desc.colors[0].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    render_pass_desc.colors[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    auto render_pass = forge::forge_render_pass_new(forge, render_pass_desc);

	/*
		auto frame = forge_frame_new(render_target);
	*/

	while (!glfwWindowShouldClose(window))
	{
		forge::forge_swapchain_update(forge, swapchain);

		/*
	        // Step 3: Define the vertex data for a triangle
            struct Vertex {
                float position[3];
            };

            Vertex vertices[] = {
                {{0.0f, -0.5f, 0.0f}},
                {{0.5f, 0.5f, 0.0f}},
                {{-0.5f, 0.5f, 0.0f}},
            };

            // Step 4: Create a Vertex Buffer
            ForgeBuffer* vertex_buffer = forge_buffer_create(
                forge,
                sizeof(vertices),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            // Update the buffer data (staging copy for device local memory)
            forge_buffer_write(forge, vertex_buffer, vertices, sizeof(vertices));

            // Step 5: Create Uniform Buffer for Model Matrix
            float model_matrix[16] = {};
            ForgeBuffer* uniform_buffer = forge_buffer_create(
                forge,
                sizeof(model_matrix),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            forge_buffer_write(forge, uniform_buffer, model_matrix, sizeof(model_matrix));

            // Step 6: Create Shader Modules
            ForgeShader* vertex_shader = forge_shader_create(
                forge,
                "triangle.vert.spv",
                VK_SHADER_STAGE_VERTEX_BIT
            );

            ForgeShader* fragment_shader = forge_shader_create(
                forge,
                "triangle.frag.spv",
                VK_SHADER_STAGE_FRAGMENT_BIT
            );

            // Step 7: Create Frame for Rendering
            ForgeFrame* frame = forge_frame_new(forge, swapchain);

            // Step 8: Begin the Frame
            forge_frame_begin(frame);

            // Step 9: Set Up Pipeline State
            forge_frame_set_vertex_shader(frame, vertex_shader);
            forge_frame_set_fragment_shader(frame, fragment_shader);
            forge_frame_set_vertex_buffer(frame, vertex_buffer);
            forge_frame_set_uniform_buffer(frame, uniform_buffer, 0); // Binding 0 for uniform buffer

            // Set Pipeline State
            ForgePipelineState pipeline_state = {};
            pipeline_state.depth_test_enable = true;
            pipeline_state.depth_write_enable = true;
            pipeline_state.depth_compare_op = VK_COMPARE_OP_LESS;
            forge_frame_set_pipeline_state(frame, pipeline_state);

            // Step 10: Draw the Triangle
            forge_frame_draw(frame, 3);  // 3 vertices to draw a triangle

            // Step 11: End the Frame and Present it
            forge_frame_end(frame);

            // Cleanup
            forge_frame_destroy(forge, frame);
            forge_shader_destroy(forge, vertex_shader);
            forge_shader_destroy(forge, fragment_shader);
            forge_buffer_destroy(forge, vertex_buffer);
            forge_buffer_destroy(forge, uniform_buffer);
            forge_swapchain_destroy(forge, swapchain);
            forge_destroy(forge);

            return 0;
            */

		glfwPollEvents();
	}

	forge_swapchain_destroy(forge, swapchain);
	forge_destroy(forge);

	return 0;
}