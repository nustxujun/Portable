
#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

Mesh::Mesh(const std::string & filename, Renderer::Ptr r)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, 0);
	if (!scene)
		abort();
	auto meshs= scene->mMeshes;

	mMeshs.resize(scene->mNumMeshes);
	for (int i = 0; i < scene->mNumMeshes; ++i)
	{
		auto m = meshs[i];
		D3D11_SUBRESOURCE_DATA InitQuadData;
		ZeroMemory(&InitQuadData, sizeof(InitQuadData));
		InitQuadData.pSysMem = m->mVertices;
		InitQuadData.SysMemPitch = 0;
		InitQuadData.SysMemSlicePitch = 0;
		auto vb = r->createBuffer(m->mNumVertices * sizeof(aiVector3D), D3D11_BIND_VERTEX_BUFFER, &InitQuadData);

		Renderer::Buffer::Ptr ib;
		std::vector<int> indices;
		if (m->HasFaces())
		{
			auto faces = m->mFaces;
			for (int j = 0; j < m->mNumFaces; ++j)
			{
				auto f = faces[j];
				for (int k = 0; k < f.mNumIndices; ++k)
					indices.push_back(f.mIndices[k]);
			}
			InitQuadData.pSysMem = indices.data();
			ib = r->createBuffer(indices.size() * sizeof(int), D3D11_BIND_INDEX_BUFFER, &InitQuadData);
		}

		mMeshs[i] = { vb,ib,m->mNumVertices ,indices.size() };
	}
}

Mesh::~Mesh()
{
}
