#include <iostream>
#include <filesystem>

#include <GLFW/glfw3.h>

#include "shader_processor.h"

constexpr uint32_t width = 800;
constexpr uint32_t height = 600;

int main()
{
	// Compile shaders
	compileShaderToSPIRV(std::filesystem::absolute("D:/Projects/Vulkan-Practice/shaders/00-Setup/main.vert"), std::filesystem::absolute("cache/o.vert.bin"));
	compileShaderToSPIRV(std::filesystem::absolute("D:/Projects/Vulkan-Practice/shaders/00-Setup/main.frag"), std::filesystem::absolute("cache/o.frag.bin"));


	if (!glfwInit())
	{
		std::cerr << "Failed to init GLFW" << std::endl;
		return 1;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, "Setup Window", nullptr, nullptr);

	if (!window)
	{
		std::cerr << "Window is Invalid." << std::endl;
		glfwTerminate();
		return 1;
	}

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}