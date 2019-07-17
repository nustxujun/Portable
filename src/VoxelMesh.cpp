#include "VoxelMesh.h"
using namespace DirectX::SimpleMath;
VoxelMesh::VoxelMesh(const Parameters & params, Renderer::Ptr r)
{
	mRenderer = r;
}

void VoxelMesh::load(size_t size,float scale, Buffer color, Buffer normal, Buffer material)
{
	const auto* colordata = color->data();

	auto getIndex = [size](int x, int y, int z)
	{
		return x + y * size + z * size * size;
	};

	auto checkExist = [colordata,size,getIndex](int x, int y, int z)
	{
		if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size)
			return false;
		auto index = getIndex(x, y, z);
		return colordata[index].w != 0;
	};

	struct Vertex
	{
		Vector3 pos;
		Vector3 norm;
		//Vector4 color;
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
				for (auto& f : faces)
				{
					if (checkExist(x + f.facing.x, y + f.facing.y, z + f.facing.z))
						continue;

					UINT startindex = vertices.size();

					for (auto& v : f.verts)
					{

						Vertex vert = { {x + v.x, y + v.y, z + v.z}, f.facing };
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
	};

	auto lo = mRenderer->createLayout(layout.data(), layout.size());

	mMeshs.push_back({
		vb,
		ib,
		vertices.size(),
		indices.size(),
		Material::Default,
		lo,
		DirectX::SimpleMath::Matrix::Identity
	});

	mAABB.min = { 0,0,0 };
	mAABB.max ={(float)size, (float)size, (float)size} ;
}
