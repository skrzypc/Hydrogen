#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "verifier.h"
#include "meshLoader.h"

namespace Hydrogen
{
	std::vector<StaticMesh> MeshLoader::Load(std::string_view path)
	{
		Assimp::Importer importer;

		const aiScene* pScene = importer.ReadFile(path.data(),
			aiProcess_Triangulate        |
			aiProcess_GenSmoothNormals   |
			aiProcess_FlipUVs            |
			aiProcess_JoinIdenticalVertices
		);

		H2_VERIFY_FATAL(pScene && !(pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && pScene->mRootNode,
			"Failed to load mesh: {}", importer.GetErrorString());

		std::vector<StaticMesh> meshes;
		meshes.reserve(pScene->mNumMeshes);

		for (uint32 i = 0; i < pScene->mNumMeshes; ++i)
		{
			const aiMesh* pMesh = pScene->mMeshes[i];

			StaticMesh& mesh = meshes.emplace_back();
			mesh.name = pMesh->mName.C_Str();

			mesh.positions.reserve(pMesh->mNumVertices);
			mesh.normals.reserve(pMesh->mNumVertices);
			mesh.uvs.reserve(pMesh->mNumVertices);

			for (uint32 v = 0; v < pMesh->mNumVertices; ++v)
			{
				mesh.positions.push_back({ pMesh->mVertices[v].x, pMesh->mVertices[v].y, pMesh->mVertices[v].z });
				mesh.normals.push_back({ pMesh->mNormals[v].x, pMesh->mNormals[v].y, pMesh->mNormals[v].z });

				if (pMesh->HasTextureCoords(0))
				{
					mesh.uvs.push_back({ pMesh->mTextureCoords[0][v].x, pMesh->mTextureCoords[0][v].y });
				}
				else
				{
					mesh.uvs.push_back({ 0.0f, 0.0f });
				}
			}

			mesh.indices.reserve(pMesh->mNumFaces * 3);
			for (uint32 f = 0; f < pMesh->mNumFaces; ++f)
			{
				const aiFace& face = pMesh->mFaces[f];
				H2_VERIFY(face.mNumIndices == 3, "Non-triangle face encountered");
				mesh.indices.push_back(face.mIndices[0]);
				mesh.indices.push_back(face.mIndices[1]);
				mesh.indices.push_back(face.mIndices[2]);
			}
		}

		return meshes;
	}
}
