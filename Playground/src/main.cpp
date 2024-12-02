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
#include <ForgeDeferedQueue.h>
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
	auto swapchain = forge::forge_swapchain_new(forge, desc);

	forge::ForgeImageDescription color_desc {};
	color_desc.name = "Color";
	color_desc.extent = {800, 600, 1};
	color_desc.type = VK_IMAGE_TYPE_2D;
	color_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
	color_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	color_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	color_desc.create_flags = 0u;
	auto color = forge::forge_image_new(forge, color_desc);

	forge::ForgeImageDescription depth_desc {};
	depth_desc.name = "Depth";
	depth_desc.extent = {800, 600, 1};
	depth_desc.type = VK_IMAGE_TYPE_2D;
	depth_desc.format = VK_FORMAT_D32_SFLOAT;
	depth_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth_desc.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depth_desc.create_flags = 0u;
	auto depth = forge::forge_image_new(forge, depth_desc);

	auto frame = forge::forge_frame_new(forge, color, depth);

	while (!glfwWindowShouldClose(window))
	{
		forge::forge_swapchain_update(forge, swapchain);

		glfwPollEvents();
	}

	forge_swapchain_destroy(forge, swapchain);
	forge_destroy(forge);

	return 0;
}