
#include "Mesh.h"
#include "MeshBuilder.h"
#include "D3D11Helper.h"

size_t Renderable::ID_GEN = 0;

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
		if (!mraw.albedo.empty())
			mat->setTexture(Material::TU_ALBEDO, r->createTexture(mraw.albedo));
		if (!mraw.normal.empty())
			mat->setTexture(Material::TU_NORMAL, r->createTexture(mraw.normal));
		//if (!mraw.ambient.empty())
		//	mat->setTexture(Material::TU_METAL, r->createTexture(mraw.ambient));
		//if (!mraw.height.empty())
		//	mat->setTexture(Material::TU_NORMAL, r->createTexture(mraw.height));
		//if (!mraw.shininess.empty())
		//	mat->setTexture(Material::TU_ROUGH, r->createTexture(mraw.shininess));

		mat->diffuse = DirectX::SimpleMath::Vector3( mraw.diffuse.data() );
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
		UINT offset = 0;
		for (auto& e : mesh.layout)
		{
			D3D11_INPUT_ELEMENT_DESC d = {0};
			switch (e)
			{
			case MeshBuilder::POSITION: d = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::NORMAL: d = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::TEXCOORD0: d = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::TANGENT: d = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::BITANGENT: d = { "TANGENT", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			case MeshBuilder::DIFFUSE: d = { "COLOR", 0 ,DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 }; break;
			default:
				abort();
				break;
			};

			offset += D3D11Helper::sizeof_DXGI_FORMAT(d.Format);
			if (d.Format != 0)
				desc.push_back(d);
		}

		auto layout = r->createLayout(desc.data(), desc.size());
		DirectX::SimpleMath::Matrix trans = (DirectX::XMFLOAT4X4)(mesh.tranfromation);
		trans.Transpose(trans);



		mMeshs.push_back({ vb, ib, mesh.numVertex, mesh.indices.size(),mesh.materialIndex == -1 ? Material::Default : materials[mesh.materialIndex] , layout , trans });
	}




}

Mesh::Mesh(std::pair<Meshs, AABB >&& meshs):
	mMeshs(std::move(meshs.first)), mAABB(meshs.second)
{
}


Mesh::~Mesh()
{
}

void Mesh::setMaterial(Material::Ptr m)
{
	for (auto&i : mMeshs)
		i.material = m;
}
