
#include "Mesh.h"
#include "MeshBuilder.h"

Mesh::Mesh(const std::string & filename, Renderer::Ptr r)
{
	MeshBuilder::build(filename);
	//mAABB = { {FLT_MAX,FLT_MAX,FLT_MAX},{FLT_MIN, FLT_MIN,FLT_MIN} };
	//Assimp::Importer importer;
	//const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	//if (!scene)
	//{
	//	MessageBoxA(NULL, importer.GetErrorString(), NULL, NULL);
	//	abort();
	//}
	//auto meshs= scene->mMeshes;

	//mMeshs.resize(scene->mNumMeshes);
	//for (int i = 0; i < scene->mNumMeshes; ++i)
	//{
	//	auto m = meshs[i];







	//	D3D11_SUBRESOURCE_DATA InitQuadData;
	//	ZeroMemory(&InitQuadData, sizeof(InitQuadData));
	//	InitQuadData.pSysMem = m->mVertices;
	//	InitQuadData.SysMemPitch = 0;
	//	InitQuadData.SysMemSlicePitch = 0;
	//	auto vb = r->createBuffer(m->mNumVertices * sizeof(aiVector3D), D3D11_BIND_VERTEX_BUFFER, &InitQuadData);

	//	for (int j = 0; j < m->mNumVertices; ++j)
	//	{
	//		auto v = m->mVertices[j];
	//		mAABB.min[0] = std::min(mAABB.min[0], v.x);
	//		mAABB.min[1] = std::min(mAABB.min[1], v.y);
	//		mAABB.min[2] = std::min(mAABB.min[2], v.z);

	//		mAABB.max[0] = std::max(mAABB.max[0], v.x);
	//		mAABB.max[1] = std::max(mAABB.max[1], v.y);
	//		mAABB.max[2] = std::max(mAABB.max[2], v.z);
	//	}

	//	Renderer::Buffer::Ptr ib;
	//	std::vector<int> indices;
	//	if (m->HasFaces())
	//	{
	//		auto faces = m->mFaces;
	//		for (int j = 0; j < m->mNumFaces; ++j)
	//		{
	//			auto f = faces[j];
	//			for (int k = 0; k < f.mNumIndices; ++k)
	//				indices.push_back(f.mIndices[k]);
	//		}
	//		InitQuadData.pSysMem = indices.data();
	//		ib = r->createBuffer(indices.size() * sizeof(int), D3D11_BIND_INDEX_BUFFER, &InitQuadData);
	//	}
	//	mMeshs[i] = { vb,ib,m->mNumVertices ,indices.size() };
	//}

}

Mesh::~Mesh()
{
}
