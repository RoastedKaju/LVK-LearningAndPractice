#include <iostream>
#include <filesystem>
#include <vector>

#include <assimp/scene.h>
#include <assimp/version.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <stb/stb_image.h>


struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
};

inline void loadModelData(const std::filesystem::path& file, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	const aiScene* scene = aiImportFile(file.string().c_str(), aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes())
	{
		std::cout << "Scene is Invalid or has no meshes\n";
		return;
	}

	const aiMesh* mesh = scene->mMeshes[0];

	// Populate vertices
	for (unsigned int i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D v = mesh->mVertices[i];
		outVertices.push_back({ .position = glm::vec3(v.x, v.y, v.z) });
	}
	// Populate indices
	for (unsigned int i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
		{
			outIndices.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}
	// UVs
	if (mesh->HasTextureCoords(0))
	{
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			const aiVector3D& uv = mesh->mTextureCoords[0][i];
			outVertices.at(i).uv = glm::vec2(uv.x, uv.y);
		}
	}

	aiReleaseImport(scene);
}

inline lvk::Holder<lvk::TextureHandle> loadTexture(const std::filesystem::path& filePath, std::unique_ptr<lvk::IContext>& ctx)
{
	int w, h, comp;
	const uint8_t* image = stbi_load(filePath.string().c_str(), &w, &h, &comp, 4);
	assert(image);

	lvk::Holder<lvk::TextureHandle> texture = ctx->createTexture({
			.type = lvk::TextureType_2D,
			.format = lvk::Format_RGBA_UN8,
			.dimensions = {(uint32_t)w, (uint32_t)h},
			.usage = lvk::TextureUsageBits_Sampled,
			.data = image,
			.debugName = "03_STB.jpg"

		});

	stbi_image_free((void*)image);

	return texture;
}