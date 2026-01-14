#pragma once

#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>
#include <imgui/imgui.h>

#include <GLFW/glfw3.h>

#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "shader_processor.h"

inline void imGuiExample()
{
	minilog::initialize(nullptr, { .threadNames = false });

	GLFWwindow* window = nullptr;
	std::unique_ptr<lvk::IContext> ctx;
	{
		LVK_PROFILER_ZONE("Initialization", LVK_PROFILER_COLOR_CREATE);
		int width = -95;
		int height = -90;

		window = lvk::initWindow("Imgui", width, height);
		ctx = lvk::createVulkanContextWithSwapchain(window, width, height, {});
		LVK_PROFILER_ZONE_END();
	}

	lvk::Holder<lvk::ShaderModuleHandle> vert = loadShaderModule(ctx, "../../../shaders/03-ImGui/main.vert");
	lvk::Holder<lvk::ShaderModuleHandle> frag = loadShaderModule(ctx, "../../../shaders/03-ImGui/main.frag");

	// Imgui
	std::unique_ptr<lvk::ImGuiRenderer> imgui = std::make_unique<lvk::ImGuiRenderer>(*ctx, "../../../fonts/OpenSans-Light.ttf", 30.0f);

	// Mouse callbacks
	glfwSetCursorPosCallback(window, [](auto* window, double x, double y) { ImGui::GetIO().MousePos = ImVec2(x, y); });
	glfwSetMouseButtonCallback(window, [](auto* window, int button, int action, int mods) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		const ImGuiMouseButton_ imguiButton = (button == GLFW_MOUSE_BUTTON_LEFT)
			? ImGuiMouseButton_Left
			: (button == GLFW_MOUSE_BUTTON_RIGHT ? ImGuiMouseButton_Right : ImGuiMouseButton_Middle);
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2((float)xpos, (float)ypos);
		io.MouseDown[imguiButton] = action == GLFW_PRESS;
		});

	// Texture loading
	int w, h, comp;
	const uint8_t* img = stbi_load("../../../models/rubber_duck/textures/Duck_baseColor.png", &w, &h, &comp, 4);

	assert(img);

	lvk::Holder<lvk::TextureHandle> texture = ctx->createTexture(
		{
			.type = lvk::TextureType_2D,
			.format = lvk::Format_RGBA_UN8,
			.dimensions = {(uint32_t)w, (uint32_t)h},
			.usage = lvk::TextureUsageBits_Sampled,
			.data = img,
			.debugName = "03_STB.jpg"
		});

	// pipelines
	lvk::Holder<lvk::RenderPipelineHandle> pipelineSoild = ctx->createRenderPipeline({
			.smVert = vert,
			.smFrag = frag,
			.color = { {.format = ctx->getSwapchainFormat() }},
			.cullMode = lvk::CullMode_Back
		});

	lvk::Holder<lvk::RenderPipelineHandle> pipelineWireframe = ctx->createRenderPipeline({
			.smVert = vert,
			.smFrag = frag,
			.color = { {.format = ctx->getSwapchainFormat() } },
			.cullMode = lvk::CullMode_Back,
			.polygonMode = lvk::PolygonMode_Line
		});

	LVK_ASSERT(pipelineSoild.valid());
	LVK_ASSERT(pipelineWireframe.valid());


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		if (!width || !height)
			continue;
		const float ratio = width / (float)height;

		const glm::mat4 m = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.5f)), (float)glfwGetTime(), glm::vec3(1.0f, 1.0f, 1.0f));
		const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		struct PerFrameData
		{
			glm::mat4 mvp;
			int isWireFrame;
		} pc = {
			.mvp = p * m,
			.isWireFrame = false
		};

		lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();
		{
			LVK_PROFILER_ZONE("Fill Command Buffer", 0xffffff);
			const lvk::Framebuffer framebuffer = {
			  .color = { {.texture = ctx->getCurrentSwapchainTexture() } },
			};
			buf.cmdBeginRendering(
				{ .color = { {.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f } } } },
				framebuffer
			);
			buf.cmdPushDebugGroupLabel("Soild Cube", 0xff0000ff);
			{
				buf.cmdBindRenderPipeline(pipelineSoild);
				buf.cmdPushConstants(pc);
				buf.cmdDraw(36);
			}
			buf.cmdPopDebugGroupLabel();

			buf.cmdPushDebugGroupLabel("Wireframe cube", 0xff0000ff);
			{
				buf.cmdBindRenderPipeline(pipelineWireframe);
				pc.isWireFrame = true;
				buf.cmdPushConstants(pc);
				buf.cmdDraw(36);
			}
			buf.cmdPopDebugGroupLabel();


			imgui->beginFrame(framebuffer);
			ImGui::Begin("Texture Viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Image(texture.index(), ImVec2(512, 512));
			ImGui::ShowDemoWindow();
			ImGui::End();
			imgui->endFrame(buf);

			LVK_PROFILER_ZONE_END();

			buf.cmdEndRendering();
		}
		ctx->submit(buf, ctx->getCurrentSwapchainTexture());
	}

	imgui = nullptr;
	texture = nullptr;

	vert.reset();
	frag.reset();
	pipelineSoild.reset();
	pipelineWireframe.reset();
	ctx.reset();

	glfwDestroyWindow(window);
	glfwTerminate();

}
