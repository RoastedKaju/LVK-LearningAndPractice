#include <iostream>
#include <filesystem>
#include <vector>

#include <assimp/scene.h>
#include <assimp/version.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>


struct Vertex
{
	glm::vec3 position;
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

	aiReleaseImport(scene);
}