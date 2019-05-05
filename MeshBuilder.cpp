#include "MeshBuilder.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <regex>
#include <functional>
MeshBuilder::Data MeshBuilder::build(const std::string & filename)
{
	Data ret;
	ret.aabb = { FLT_MAX,FLT_MAX,FLT_MAX,FLT_MIN, FLT_MIN,FLT_MIN };
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	if (!scene)
	{
		abort();
		return ret;
	}


	std::regex reg("^(.+)[/\\\\].+\\..+$");
	std::smatch match;
	std::string totalpath;
	if (std::regex_match(filename, match, reg))
	{
		totalpath = std::string(match[1]) + "/";
	}

	std::function<void(const aiNode*, const aiMatrix4x4* ,std::vector<Data::Mesh>&)> process;
	process = [&process,scene,&ret](const aiNode* node, const aiMatrix4x4*  pm, std::vector<Data::Mesh>& rets)
	{
		aiMatrix4x4 trans = node->mTransformation;
		if (pm)
		{
			trans = *pm * trans;
		}
		for (int i = 0; i < node->mNumChildren; ++i)
		{
			process(node->mChildren[i], &trans, rets);
		}
		auto meshs = node->mMeshes;

		for (int i = 0; i < node->mNumMeshes; ++i)
		{
			Data::Mesh mesh;
			mesh.layout.push_back(POSITION);
			auto m = scene->mMeshes[ meshs[i] ];
			mesh.materialIndex = m->mMaterialIndex;
			mesh.numVertex = m->mNumVertices;
			memcpy(&mesh.tranfromation, &trans, sizeof(mesh.tranfromation));

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
				aiVector3D pos = *(m->mVertices + j);
				pos = trans * pos;
				copy(begin, &pos, sizeof(aiVector3D));

				ret.aabb[0] = std::min(ret.aabb[0], pos.x);
				ret.aabb[1] = std::min(ret.aabb[1], pos.y);
				ret.aabb[2] = std::min(ret.aabb[2], pos.z);

				ret.aabb[3] = std::max(ret.aabb[3], pos.x);
				ret.aabb[4] = std::max(ret.aabb[4], pos.y);
				ret.aabb[5] = std::max(ret.aabb[5], pos.z);


				if (m->HasNormals())
				{
					copy(begin, m->mNormals + j, sizeof(aiVector3D));
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

			rets.push_back(std::move(mesh));
		}

	};

	process(scene->mRootNode,nullptr, ret.meshs);





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
				std::string realpath = totalpath + path.C_Str();
				mat.texture_diffuses.push_back(realpath);
			}

			ret.materials.push_back(mat);
		}
	}
	return ret;
}
