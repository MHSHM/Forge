#include <iostream>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Forge.h>
#include <ForgeLogger.h>
#include <ForgeSwapchain.h>

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

	while (!glfwWindowShouldClose(window))
	{
		forge::forge_swapchain_update(forge, swapchain);

		glfwPollEvents();
	}

	forge_swapchain_destroy(forge, swapchain);
	forge_destroy(forge);

	return 0;
}