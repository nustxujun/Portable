
#include "Mesh.h"
#include "MeshBuilder.h"

Mesh::Mesh(const Parameters& params, Renderer::Ptr r)
{

	auto end = params.end();
	auto filename = params.find("file");
	if (filename == end)
		return;


	auto data = MeshBuilder::build(filename->second);
	

	std::vector<Material::Ptr> materials;
	for (int i = 0; i < data.materials.size(); ++i)
	{
		auto mraw = data.materials[i];
		Material* mat = new Material();
		for (int j = 0; j < mraw.texture_diffuses.size(); ++j)
		{
			mat->setTexture(j, r->createTexture(mraw.texture_diffuses[j]));
		}
		materials.emplace_back(mat);
	}

	for (auto& mesh : data.meshs)
	{
		D3D11_SUBRESOURCE_DATA InitQuadData;
		ZeroMemory(&InitQuadData, sizeof(InitQuadData));
		InitQuadData.pSysMem = mesh.vertices.data();
		InitQuadData.SysMemPitch = 0;
		InitQuadData.SysMemSlicePitch = 0;
		auto vb = r->createBuffer(mesh.vertices.size(), D3D11_BIND_VERTEX_BUFFER, &InitQuadData);

		InitQuadData.pSysMem = mesh.indices.data();
		auto ib = r->createBuffer(mesh.indices.size() * sizeof(int), D3D11_BIND_INDEX_BUFFER, &InitQuadData);

		size_t stride = 0;
		for (auto & i : mesh.layout)
		{
			switch (i)
			{
			case MeshBuilder::POSITION: stride += 4 * 3; break;
			case MeshBuilder::NORMAL: stride += 4 * 3; break;
			case MeshBuilder::TEXCOORD0: stride += 4 * 2; break;

			default:
				break;
			}
		}

		mMeshs.push_back({ vb, ib, mesh.numVertex, mesh.indices.size(), materials[mesh.materialIndex],stride });
	}





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


}

Mesh::~Mesh()
{
}
