#include "VoxelMesh.h"
using namespace DirectX::SimpleMath;
VoxelMesh::VoxelMesh(const Parameters & params, Renderer::Ptr r)
{
	mRenderer = r;
}

void VoxelMesh::load(size_t size,float scale, Buffer color, Buffer normal, Buffer material)
{
	const UINT* colordata = (const UINT*)color->data();
	const UINT* normaldata = (const UINT*)normal->data();


	auto uintTofloat4 = [](UINT val) {
		return Vector4(Vector4(float((val & 0x000000FF)),
			float((val & 0x0000FF00) >> 8U),
			float((val & 0x00FF0000) >> 16U),
			float((val & 0xFF000000) >> 24U)) / 255);
	};

	auto decodeNormal = [uintTofloat4](UINT n)
	{
		Vector4 data = uintTofloat4(n);
		Vector3 norm = { data.x * 2 - 1, data.y * 2 - 1, data.z * 2 - 1 };
		norm.Normalize();
		return norm;
	};


	auto getIndex = [size](int x, int y, int z)
	{
		return x + y * size + z * size * size;
	};

	auto checkExist = [uintTofloat4,colordata,size,getIndex](int x, int y, int z)
	{
		if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size)
			return false;
		auto index = getIndex(x, y, z);
		return uintTofloat4(colordata[index]).w != 0;
	};

	struct Vertex
	{
		Vector3 pos;
		Vector3 norm;
		Vector4 color;
	};

	std::vector<Vertex> vertices;
	std::vector<UINT> indices;


	struct Face
	{
		Vector3 facing;
		std::array<Vector3, 4> verts;
	};
	std::array<Face, 6> faces;

	faces[0] = {
		Vector3(1.0f, 0, 0),
		{ Vector3(1.0f, 0,0), Vector3(1.0f,1.0f,0), Vector3(1.0f,1.0f,1.0f), Vector3(1.0f,0,1.0f) },
	};

	faces[1] = {
		Vector3(-1.0f, 0, 0),
		{ Vector3(0,0,0), Vector3(0,0,1.0f), Vector3(0,1.0f,1.0f), Vector3(0,1.0f,0) }
	};
	faces[2] = {
			Vector3(0,1.0f,0),
			{Vector3(0,1.0f,0), Vector3(0,1.0f,1.0f), Vector3(1.0f,1.0f,1.0f), Vector3(1.0f,1.0f,0)}
	};
	faces[3] = {
			Vector3(0, -1.0f, 0),
			{ Vector3(0,0,0), Vector3(1.0f,0,0), Vector3(1.0f,0,1.0f), Vector3(0,0,1.0f) },
	};
	faces[4] = {
			Vector3(0,0,1.0f),
			{Vector3(0,0,1.0f),Vector3(1.0f,0,1.0f), Vector3(1.0f,1.0f,1.0f),Vector3(0,1.0f,1.0f)}
	};
	faces[5] = {
			Vector3(0,0,-1.0f),
			{Vector3(0,0,0), Vector3(0,1.0f,0),Vector3(1.0f,1.0f,0),Vector3(1.0f,0,0)}
	};


	for (int z = 0; z < size; ++z)
	{
		for (int y = 0; y < size; ++y)
		{
			for (int x = 0; x < size; ++x)
			{
				int index = getIndex(x, y, z);
				if (!checkExist(x, y, z))
					continue;
			
				auto color = uintTofloat4(colordata[index]) ;
				auto normal = decodeNormal(normaldata[index]);
				for (auto& f : faces)
				{
					if (checkExist(x + f.facing.x, y + f.facing.y, z + f.facing.z))
						continue;

					UINT startindex = vertices.size();

					for (auto& v : f.verts)
					{

						Vertex vert = { 
							{x + v.x, y + v.y, z + v.z}, 
							//f.facing ,
							normal,
							color };
						vert.pos *= scale;
						vertices.push_back(vert);

					}

					indices.push_back(0 + startindex);
					indices.push_back(1 + startindex);
					indices.push_back(2 + startindex);

					indices.push_back(0 + startindex);
					indices.push_back(2 + startindex);
					indices.push_back(3 + startindex);
				}
			}
		}
	}

	D3D11_SUBRESOURCE_DATA data = {0};
	data.pSysMem = vertices.data();

	auto vb = mRenderer->createBuffer(sizeof(Vertex) * vertices.size(), D3D11_BIND_VERTEX_BUFFER, &data);

	data.pSysMem = indices.data();
	auto ib = mRenderer->createBuffer(indices.size() * sizeof(int), D3D11_BIND_INDEX_BUFFER, &data);

	std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0 ,DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	auto lo = mRenderer->createLayout(layout.data(), layout.size());

	static Material::Ptr mat = Material::create();
	mat->usingVertexColor = true;
	mat->metallic = 1;
	mMeshs.push_back({
		vb,
		ib,
		vertices.size(),
		indices.size(),
		mat,
		lo,
		DirectX::SimpleMath::Matrix::Identity
	});

	mAABB.min = { 0,0,0 };
	mAABB.max ={(float)size * scale, (float)size* scale, (float)size* scale } ;
}
