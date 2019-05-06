#include "MeshBuilder.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <regex>
#include <functional>
#include <Windows.h>
#undef min
#undef max

#include "tiny_obj_loader.h"

MeshBuilder::Data MeshBuilder::build(const std::string & filename)
{
	std::regex obj("^.+\\.obj$");
	if (std::regex_match(filename, obj))
		return buildByTinyobj(filename);
	else
		return buildByAssimp(filename);


}

MeshBuilder::Data MeshBuilder::buildByAssimp(const std::string & filename)
{

	Data ret;
	ret.aabb = { FLT_MAX,FLT_MAX,FLT_MAX,FLT_MIN, FLT_MIN,FLT_MIN };
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	if (!scene)
	{
		::MessageBoxA(NULL, importer.GetErrorString(), NULL, NULL);
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
				UINT index;
				m->GetTexture(aiTextureType_DIFFUSE, j, &path, &mapping, &index);
				std::string realpath = totalpath + path.C_Str();
				mat.texture_diffuses.push_back(realpath);
			}

			ret.materials.push_back(mat);
		}
	}

	std::function<void(const aiNode*, const aiMatrix4x4*, std::vector<Data::Mesh>&)> process;
	process = [&process, scene, &ret](const aiNode* node, const aiMatrix4x4*  pm, std::vector<Data::Mesh>& rets)
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
			auto m = scene->mMeshes[meshs[i]];
			mesh.materialIndex = m->mMaterialIndex;
			bool hasMaterial = ret.materials.size() > mesh.materialIndex;
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

	process(scene->mRootNode, nullptr, ret.meshs);






	return ret;
}

MeshBuilder::Data MeshBuilder::buildByTinyobj(const std::string & filename)
{
	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(filename))
	{
		MessageBoxA(NULL, reader.Error().c_str(), NULL, NULL);
		abort();
	}

	Data ret;

	const auto* vertices =reader.GetAttrib().vertices.data();
	const auto* normals = reader.GetAttrib().normals.data();
	const auto* texcoords = reader.GetAttrib().texcoords.data();


	for (auto& i : reader.GetMaterials())
	{
		Data::Material m;
		m.texture_diffuses.push_back(i.diffuse_texname);
		ret.materials.push_back(m);
	}

	// Loop over shapes
	for (size_t s = 0; s < reader.GetShapes().size(); s++) {
		const auto& shapes = reader.GetShapes();
		// Loop over faces(polygon)
		size_t index_offset = 0;
		Data::Mesh mesh;
		auto copy = [](char*& buffer, const void* src, int size) {
			memcpy(buffer, src, size);
			buffer += size;
		};
		mesh.layout = { POSITION, NORMAL ,TEXCOORD0 };
		mesh.numVertex = shapes[s].mesh.num_face_vertices.size() * 3;
		mesh.vertices.resize(mesh.numVertex * 32);
		mesh.materialIndex = shapes[s].mesh.material_ids[0];
		char* data = mesh.vertices.data();
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			if (fv != 3)
				abort();
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				mesh.indices.push_back(idx.vertex_index);

				char* head = data + idx.vertex_index * 32;
				copy(head, vertices + 3 * idx.vertex_index, 12);
				copy(head, normals + 3 * idx.normal_index, 12);
				copy(head, texcoords + 2 * idx.normal_index, 8);

			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}


	return Data();
}
