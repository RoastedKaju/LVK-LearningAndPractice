#pragma once

#include "lvk/LVK.h"

#include "shader_processor.h"
#include "model_loader.h"
#include "Bitmap.h"
#include "UtilsCubemap.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <vector>
#include <memory>

inline void cubemap()
{
	minilog::initialize(nullptr, { .threadNames = false });

	GLFWwindow* window = nullptr;

	std::unique_ptr<lvk::IContext> ctx;
	lvk::Holder<lvk::TextureHandle> depthTexture;

	int width = -95;
	int height = -90;

	window = lvk::initWindow("Simple Example", width, height);
	ctx = lvk::createVulkanContextWithSwapchain(window, width, height, {});

	depthTexture = ctx->createTexture(
		{
			.type = lvk::TextureType_2D,
			.format = lvk::Format_Z_F32,
			.dimensions = {(uint32_t)width, (uint32_t)height},
			.usage = lvk::TextureUsageBits_Attachment,
			.debugName = "Depth buffer"
		});

	struct VertexData
	{
		glm::vec3 pos;
		glm::vec3 n;
		glm::vec2 tc;
	};

	lvk::Holder<lvk::ShaderModuleHandle> vert = loadShaderModule(ctx, "../../../shaders/03-ImGui/main_v2.vert");
	lvk::Holder<lvk::ShaderModuleHandle> frag = loadShaderModule(ctx, "../../../shaders/03-ImGui/main_v2.frag");
	lvk::Holder<lvk::ShaderModuleHandle> vertSkybox = loadShaderModule(ctx, "../../../shaders/03-ImGui/skybox.vert");
	lvk::Holder<lvk::ShaderModuleHandle> fragSkybox = loadShaderModule(ctx, "../../../shaders/03-ImGui/skybox.frag");

	const lvk::VertexInput vdesc = {
	  .attributes = {	{.location = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(VertexData, pos) },
						{.location = 1, .format = lvk::VertexFormat::Float3, .offset = offsetof(VertexData, n) },
						{.location = 2, .format = lvk::VertexFormat::Float2, .offset = offsetof(VertexData, tc) }, },
	  .inputBindings = { {.stride = sizeof(VertexData) } },
	};

	// Pipelines
	lvk::Holder<lvk::RenderPipelineHandle> pipeline = ctx->createRenderPipeline({
		   .vertexInput = vdesc,
		   .smVert = vert,
		   .smFrag = frag,
		   .color = { {.format = ctx->getSwapchainFormat() } },
		   .depthFormat = ctx->getFormat(depthTexture),
		   .cullMode = lvk::CullMode_Back,
		});

	lvk::Holder<lvk::RenderPipelineHandle> pipelineSkybox = ctx->createRenderPipeline({
		.smVert = vertSkybox,
		.smFrag = fragSkybox,
		.color = { {.format = ctx->getSwapchainFormat() } },
		.depthFormat = ctx->getFormat(depthTexture),
		});

	const aiScene* scene = aiImportFile("../../../models/rubber_duck/scene.gltf", aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes()) {
		printf("Unable to load data/rubber_duck/scene.gltf\n");
		exit(255);
	}

	// Model Loading
	const aiMesh* mesh = scene->mMeshes[0];
	std::vector<VertexData> vertices;
	for (uint32_t i = 0; i != mesh->mNumVertices; i++) {
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D n = mesh->mNormals[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		vertices.push_back({ .pos = glm::vec3(v.x, v.y, v.z), .n = glm::vec3(n.x, n.y, n.z), .tc = glm::vec2(t.x, t.y) });
	}
	std::vector<uint32_t> indices;
	for (uint32_t i = 0; i != mesh->mNumFaces; i++) {
		for (uint32_t j = 0; j != 3; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	aiReleaseImport(scene);

	const size_t kSizeIndices = sizeof(uint32_t) * indices.size();
	const size_t kSizeVertices = sizeof(VertexData) * vertices.size();

	// indices
	lvk::Holder<lvk::BufferHandle> bufferIndices = ctx->createBuffer(
		{ .usage = lvk::BufferUsageBits_Index,
		  .storage = lvk::StorageType_Device,
		  .size = kSizeIndices,
		  .data = indices.data(),
		  .debugName = "Buffer: indices" },
		nullptr);

	// vertices
	lvk::Holder<lvk::BufferHandle> bufferVertices = ctx->createBuffer(
		{ .usage = lvk::BufferUsageBits_Vertex,
		  .storage = lvk::StorageType_Device,
		  .size = kSizeVertices,
		  .data = vertices.data(),
		  .debugName = "Buffer: vertices" },
		nullptr);


	struct PerFrameData
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 cameraPos;
		uint32_t tex = 0;
		uint32_t texCube = 0;
	};

	lvk::Holder<lvk::BufferHandle> bufferPerFrame = ctx->createBuffer(
		{ .usage = lvk::BufferUsageBits_Uniform,
		  .storage = lvk::StorageType_Device,
		  .size = sizeof(PerFrameData),
		  .debugName = "Buffer: per-frame" },
		nullptr);

	// texture
	lvk::Holder<lvk::TextureHandle> texture = loadTexture(std::filesystem::absolute("../../../models/rubber_duck/textures/Duck_baseColor.png"), ctx);

	// cube map
	lvk::Holder<lvk::TextureHandle> cubemapTex;
	{
		int w, h;
		const float* img = stbi_loadf("../../../HDR/piazza_bologni_1k.hdr", &w, &h, nullptr, 4);
		Bitmap in(w, h, 4, eBitmapFormat_Float, img);
		Bitmap out = convertEquirectangularMapToVerticalCross(in);
		stbi_image_free((void*)img);

		stbi_write_hdr(".cache/screenshot.hdr", out.w_, out.h_, out.comp_, (const float*)out.data_.data());

		Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

		cubemapTex = ctx->createTexture({
			.type = lvk::TextureType_Cube,
			.format = lvk::Format_RGBA_F32,
			.dimensions = {(uint32_t)cubemap.w_, (uint32_t)cubemap.h_},
			.usage = lvk::TextureUsageBits_Sampled,
			.data = cubemap.data_.data(),
			.debugName = "data/piazza_bologni_1k.hdr",
			});
	}

	// window loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		if (!width || !height)
			continue;

		const float ratio = width / (float)height;

		const glm::vec3 cameraPos(0.0f, 1.0f, -1.5f);

		const glm::mat4 p = glm::perspective(glm::radians(60.0f), ratio, 0.1f, 1000.0f);
		const glm::mat4 m1 = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
		const glm::mat4 m2 = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 v = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		const lvk::RenderPass renderPass = {
			.color = { {.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f } } },
			.depth = {.loadOp = lvk::LoadOp_Clear, .clearDepth = 1.0f }
		};

		const lvk::Framebuffer framebuffer = {
		  .color = { {.texture = ctx->getCurrentSwapchainTexture() } },
		  .depthStencil = {.texture = depthTexture },
		};

		lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();
		buf.cmdUpdateBuffer(
			bufferPerFrame, PerFrameData{
								.model = m2 * m1,
								.view = v,
								.proj = p,
								.cameraPos = glm::vec4(cameraPos, 1.0f),
								.tex = texture.index(),
								.texCube = cubemapTex.index(),
			});

		{
			buf.cmdBeginRendering(renderPass, framebuffer);
			{
				{
					buf.cmdPushDebugGroupLabel("Skybox", 0xff0000ff);
					buf.cmdBindRenderPipeline(pipelineSkybox);
					buf.cmdPushConstants(ctx->gpuAddress(bufferPerFrame));
					buf.cmdDraw(36);
					buf.cmdPopDebugGroupLabel();
				}
				{
					buf.cmdPushDebugGroupLabel("Mesh", 0xff0000ff);
					buf.cmdBindVertexBuffer(0, bufferVertices);
					buf.cmdBindRenderPipeline(pipeline);
					buf.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
					buf.cmdBindIndexBuffer(bufferIndices, lvk::IndexFormat_UI32);
					buf.cmdDrawIndexed(indices.size());
					buf.cmdPopDebugGroupLabel();
				}
				buf.cmdEndRendering();
			}
		}
		ctx->submit(buf, ctx->getCurrentSwapchainTexture());
	}

	vert.reset();
	frag.reset();
	vertSkybox.reset();
	fragSkybox.reset();

	texture.reset();
	cubemapTex.reset();
	depthTexture.reset();

	bufferVertices.reset();
	bufferIndices.reset();
	bufferPerFrame.reset();

	pipeline.reset();
	pipelineSkybox.reset();

	ctx.reset();

	glfwDestroyWindow(window);
	glfwTerminate();
}