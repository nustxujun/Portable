
#include "Mesh.h"
#include "MeshBuilder.h"
#include "D3D11Helper.h"

Mesh::Mesh(const Parameters& params, Renderer::Ptr r)
{

	auto end = params.end();
	auto filename = params.find("file");
	if (filename == end)
		return;

	auto data = MeshBuilder::build(filename->second);

	memcpy(&mAABB, &data.aabb, sizeof(mAABB));

	std::vector<Material::Ptr> materials;
	for (int i = 0; i < data.materials.size(); ++i)
	{
		auto mraw = data.materials[i];
		Material* mat = new Material();
		for (int j = 0; j < mraw.texture_diffuses.size(); ++j)
		{
			mat->setTexture(j, r->createTexture( mraw.texture_diffuses[j]));
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

		std::vector<D3D11_INPUT_ELEMENT_DESC> desc;
		size_t offset = 0;
		for (auto& e : mesh.layout)
		{
			D3D11_INPUT_ELEMENT_DESC d = {0};
			switch (e)
			{
			case MeshBuilder::POSITION: d = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::NORMAL: d = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::TEXCOORD0: d = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			default:
				break;
			};

			offset += D3D11Helper::sizeof_DXGI_FORMAT(d.Format);
			if (d.Format != 0)
				desc.push_back(d);
		}

		auto layout = r->createLayout(desc.data(), desc.size());
		DirectX::SimpleMath::Matrix trans = (DirectX::XMFLOAT4X4)(mesh.tranfromation);
		trans.Transpose(trans);

		mMeshs.push_back({ vb, ib, mesh.numVertex, mesh.indices.size(), materials[mesh.materialIndex] , layout , trans });
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
