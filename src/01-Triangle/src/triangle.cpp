#include <iostream>
#include <filesystem>

#include <lvk/LVK.h>
#include <GLFW/glfw3.h>

#include "shader_processor.h"

int main()
{
	minilog::initialize(nullptr, { .threadNames = false });

	// Giving non-positive numbers will let the window automatically pick the size
	int width = -95;
	int height = -90;

	// We will use LVK's window instead
	GLFWwindow* window = lvk::initWindow("Simple Window", width, height);

	// Just like glfwMakeContextCurrent(window);
	std::unique_ptr<lvk::IContext> ctx = lvk::createVulkanContextWithSwapchain(window, width, height, {});

	lvk::Holder<lvk::ShaderModuleHandle> vert = loadShaderModule(ctx, std::filesystem::absolute("D:/Projects/Vulkan-Practice/shaders/00-Triangle/main.vert"));
	lvk::Holder<lvk::ShaderModuleHandle> frag = loadShaderModule(ctx, std::filesystem::absolute("D:/Projects/Vulkan-Practice/shaders/00-Triangle/main.frag"));

	// Pipeline (Everything like Shaders, Blending, Formats is baked into the pipeline)
	lvk::Holder<lvk::RenderPipelineHandle> soildPipeline = ctx->createRenderPipeline({
		.smVert = vert,
		.smFrag = frag,
		.color = {{.format = ctx->getSwapchainFormat()}}

		});

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glfwGetFramebufferSize(window, &width, &height);
		// Skip rendering if width or height is zero
		if (!width || !height)
			continue;

		// You record commands and submit them
		lvk::ICommandBuffer& buff = ctx->acquireCommandBuffer();
		// Clear color, bind frame buffer etc
		buff.cmdBeginRendering(
			{ .color = { {.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f } } } },
			{ .color = { {.texture = ctx->getCurrentSwapchainTexture() } } });
		// glUseProgram
		buff.cmdBindRenderPipeline(soildPipeline);
		buff.cmdPushDebugGroupLabel("Render Triangle", 0xff0000ff);
		// glDrawArrays(GL_TRIANGLES, 0, 3)
		buff.cmdDraw(3);
		buff.cmdPopDebugGroupLabel();
		buff.cmdEndRendering();

		//glfwSwapBuffers
		ctx->submit(buff, ctx->getCurrentSwapchainTexture());
	}

	soildPipeline.reset();
	frag.reset();
	vert.reset();
	ctx.reset();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}