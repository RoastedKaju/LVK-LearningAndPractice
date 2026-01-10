#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#include <lvk/LVK.h>
#include <GLFW/glfw3.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include "shader_processor.h"
#include "model_loader.h"

int main()
{
	minilog::initialize(nullptr, { .threadNames = false });

	int width = -95;
	int height = -90;

	GLFWwindow* window = lvk::initWindow("Model", width, height);

	// Context
	std::unique_ptr<lvk::IContext> ctx = lvk::createVulkanContextWithSwapchain(window, width, height, {});

	std::vector<Vertex> verts;
	std::vector<uint32_t> indices;

	// Call load model data function
	loadModelData(std::filesystem::absolute("../../../models/rubber_duck/scene.gltf"), verts, indices);
	// Call load texture function
	lvk::Holder<lvk::TextureHandle> texture = loadTexture(std::filesystem::absolute("../../../models/rubber_duck/textures/Duck_baseColor.png"), ctx);

	// Vertex Buffer
	lvk::Holder<lvk::BufferHandle> vertexBuffer = ctx->createBuffer({
		.usage = lvk::BufferUsageBits_Vertex,
		.storage = lvk::StorageType_Device,
		.size = sizeof(Vertex) * verts.size(),
		.data = verts.data(),
		.debugName = "Buffer: vertex"
		});
	// Index Buffer
	lvk::Holder<lvk::BufferHandle> indexBuffer = ctx->createBuffer({
		.usage = lvk::BufferUsageBits_Index,
		.storage = lvk::StorageType_Device,
		.size = sizeof(uint32_t) * indices.size(),
		.data = indices.data(),
		.debugName = "Buffer: index"
		});

	// Depth Texture
	lvk::Holder<lvk::TextureHandle> depthTexture = ctx->createTexture({
		.type = lvk::TextureType_2D,
		.format = lvk::Format_Z_F32,
		.dimensions = {(uint32_t)width, (uint32_t)height},
		.usage = lvk::TextureUsageBits_Attachment,
		.debugName = "Depth buffer"
		});

	// Attribute pointer
	const lvk::VertexInput vdesc = {
		.attributes = { 
			{
				.location = 0,
				.format = lvk::VertexFormat::Float3,
				.offset = offsetof(Vertex, position)
			},
			{
				.location = 1,
				.format = lvk::VertexFormat::Float2,
				.offset = offsetof(Vertex, uv)
			}
		},
		.inputBindings = { {.stride = sizeof(Vertex) } }
	};

	// Shaders
	lvk::Holder<lvk::ShaderModuleHandle> vert = loadShaderModule(ctx, std::filesystem::absolute("../../../shaders/02-Model/main.vert"));
	lvk::Holder<lvk::ShaderModuleHandle> frag = loadShaderModule(ctx, std::filesystem::absolute("../../../shaders/02-Model/main.frag"));

	// Render piplines
	// Soild pipeline
	lvk::Holder<lvk::RenderPipelineHandle> pipelineSolid = ctx->createRenderPipeline({
			.vertexInput = vdesc,
			.smVert = vert,
			.smFrag = frag,
			.color = { {.format = ctx->getSwapchainFormat() } },
			.depthFormat = ctx->getFormat(depthTexture),
			.cullMode = lvk::CullMode_Back
		});

	const uint32_t isWireframe = 1;

	// Wireframe pipeline
	lvk::Holder<lvk::RenderPipelineHandle> pipelineWireframe = ctx->createRenderPipeline({
			  .vertexInput = vdesc,
			  .smVert = vert,
			  .smFrag = frag,
			  .specInfo = {.entries = { {.constantId = 0, .size = sizeof(uint32_t) } }, .data = &isWireframe, .dataSize = sizeof(isWireframe) },
			  .color = { {.format = ctx->getSwapchainFormat() } },
			  .depthFormat = ctx->getFormat(depthTexture),
			  .cullMode = lvk::CullMode_Back,
			  .polygonMode = lvk::PolygonMode_Line,
		});

	LVK_ASSERT(pipelineSolid.valid());
	LVK_ASSERT(pipelineWireframe.valid());

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		if (!width || !height)
		{
			continue;
		}

		const float ratio = width / (float)height;

		const glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
		const glm::mat4 v = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -1.5f)), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		// Clear color
		const lvk::RenderPass renderPass = {
			.color = {{.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f }}},
			.depth = {.loadOp = lvk::LoadOp_Clear, .clearDepth = 1.0f}
		};

		const lvk::Framebuffer framebuffer = {
			.color = { { .texture = ctx->getCurrentSwapchainTexture() } },
			.depthStencil = { .texture = depthTexture }
		};

		// Perframe data
		const struct PerFrameData
		{
			glm::mat4 mvp;
			uint32_t textureId;
		} pc = {
			.mvp = p * v * m,
			.textureId = texture.index()
		};

		lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();
		{
			buf.cmdBeginRendering(renderPass, framebuffer);
			buf.cmdPushDebugGroupLabel("Mesh", 0xff0000ff);
			{
				buf.cmdBindVertexBuffer(0, vertexBuffer);
				buf.cmdBindIndexBuffer(indexBuffer, lvk::IndexFormat_UI32);
				buf.cmdBindRenderPipeline(pipelineSolid);
				buf.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
				buf.cmdPushConstants(pc);
				buf.cmdDrawIndexed(indices.size());
				buf.cmdBindRenderPipeline(pipelineWireframe);
				buf.cmdSetDepthBiasEnable(true);
				buf.cmdSetDepthBias(0.0f, -1.0f, 0.0f);
				buf.cmdDrawIndexed(indices.size());
			}
			buf.cmdPopDebugGroupLabel();
			buf.cmdEndRendering();
		}

		ctx->submit(buf, ctx->getCurrentSwapchainTexture());
	}


	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}