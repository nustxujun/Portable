#include "MeshBuilder.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

MeshBuilder::Data MeshBuilder::build(const std::string & filename)
{
	Data ret;
	//ret.aabb = { FLT_MAX,FLT_MAX,FLT_MAX,FLT_MIN, FLT_MIN,FLT_MIN };
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	if (!scene)
	{
		return ret;
	}
	auto meshs = scene->mMeshes;


	for (int i = 0; i < scene->mNumMeshes; ++i)
	{
		Data::Mesh mesh;
		mesh.layout.push_back(POSITION);
		auto m = meshs[i];
		mesh.materialIndex = m->mMaterialIndex;
		mesh.numVertex = m->mNumVertices;

		size_t size = 4 * 3;
		if (m->HasNormals())
		{
			mesh.layout.push_back(NORMAL);
			size += 4 * 3;
		}
		if (m->HasTextureCoords(0))
		{
			mesh.layout.push_back(TEXCOORD0);
			size += 4 * 2;
		}

		mesh.vertices.resize(mesh.numVertex * size);
		char* begin = mesh.vertices.data();
		auto copy = [](char*& buffer, const void* cont, size_t size) {
			memcpy(buffer, cont, size);
			buffer += size;
		};
		for (int j = 0; j < m->mNumVertices; ++j)
		{
			copy(begin, m->mVertices + j , sizeof(aiVector3D));

			if (m->HasNormals())
			{
				copy(begin, m->mNormals + j , sizeof(aiVector3D) );
			}

			if (m->HasTextureCoords(0))
			{
				copy(begin, m->mTextureCoords[0] + j, 4 * 2);
			}
		}

		if (m->HasFaces())
		{
			for (int j = 0; j < m->mNumFaces; ++j)
			{
				for (int k = 0; k < 3; ++k)
					mesh.indices.push_back(m->mFaces[j].mIndices[k]);
			}
		}

		
		ret.meshs.push_back(std::move(mesh));
	}

	if (scene->HasMaterials())
	{
		for (int i = 0; i < scene->mNumMaterials; ++i)
		{
			auto m = scene->mMaterials[i];
			Data::Material mat;
			for (int j = 0; j < m->GetTextureCount(aiTextureType_DIFFUSE); ++j)
			{
				aiString path;
				aiTextureMapping mapping;
				size_t index;
				m->GetTexture(aiTextureType_DIFFUSE, j, &path,&mapping, &index);
				mat.texture_diffuses.push_back(path.C_Str());
			}

			ret.materials.push_back(mat);
		}
	}
	return ret;
}
