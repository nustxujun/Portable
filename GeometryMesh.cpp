#include "GeometryMesh.h"
#include <sstream>
GeometryMesh::GeometryMesh(const Parameters & params, Renderer::Ptr r):Mesh(generateGeometry(params,r))
{
}

GeometryMesh::~GeometryMesh()
{
}

std::pair<Mesh::Meshs, Mesh::AABB> GeometryMesh::generateGeometry(const Parameters & params, Renderer::Ptr r)
{
	using namespace DirectX::SimpleMath;
	auto geom = params.find("geom");
	auto end = params.end();
	if (end == geom)
		return {};

	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	AABB aabb = { {FLT_MAX,FLT_MAX,FLT_MAX},{FLT_MIN, FLT_MIN,FLT_MIN} };

	DirectX::SimpleMath::Matrix trans = DirectX::SimpleMath::Matrix::Identity;
	if (geom->second == "sphere")
	{
		float radius = 5;
		int resolution = 100;
		if (params.find("radius") != end)
		{
			std::stringstream ss;
			ss << params.find("radius")->second;
			ss >> radius;
		}

		if (params.find("resolution") != end)
		{
			resolution = atoi(params.find("resolution")->second.c_str());
		}

		const float pi = 3.14159265358;
		size_t vcount = 0;
		for (int j = 0; j <= resolution; ++j)
		{
			float degy = pi * j / resolution;
			float siny = sin(degy);
			float cosy = cos(degy);
			float y = cosy * radius;
			float r = siny * radius;
			//float r = radius;
			for (int i = 0; i <= resolution; ++i)
			{
				float degx = i * 2 * pi / resolution;
				float sinx = sin(degx);
				float cosx = cos(degx);

				float z = cosx * r;
				float x = sinx * r;

				// pos
				vertices.push_back(x);
				vertices.push_back(y);
				vertices.push_back(z);

				// normal
				Vector3 normal = { x,y,z };
				normal.Normalize();
				vertices.push_back(normal.x);
				vertices.push_back(normal.y);
				vertices.push_back(normal.z);

				//uv
				vertices.push_back((float)i/(float)resolution);
				vertices.push_back((float)j / (float)resolution);

				// tangent
				Vector3 tangent = Vector3::UnitY.Cross(normal);
				tangent.Normalize();
				vertices.push_back(tangent.x);
				vertices.push_back(tangent.y);
				vertices.push_back(tangent.z);

				//bitangent
				Vector3 bitangent = tangent.Cross(normal);
				bitangent.Normalize();
				vertices.push_back(bitangent.x);
				vertices.push_back(bitangent.y);
				vertices.push_back(bitangent.z);

			}
		}


		for (int j = 0; j < resolution; ++j)
		{
			for (int i = 0; i < resolution; ++i)
			{
				unsigned int p1 = j * (resolution + 1) + i;
				unsigned int p2 = p1 + (resolution + 1);

				indices.push_back(p1);
				indices.push_back(p2);
				indices.push_back(p1 + 1);

				indices.push_back(p1 + 1);
				indices.push_back(p2);
				indices.push_back(p2 + 1);
			}
		}

	}
	else if (geom->second == "plane")
	{
		int size = 10;
		int resolution = 1;

		if (params.find("size") != end)
		{
			size = atoi(params.find("size")->second.c_str());
		}

		//if (params.find("resolution") != end)
		//{
		//	resolution = atoi(params.find("resolution")->second.c_str());
		//}

		trans.Translation({ -size * 0.5f, 0, -size * 0.5f });

		float tilesize = (float)size / (float)resolution;
		for (int j = 0; j <= resolution; ++j)
		{
			for (int i = 0; i <= resolution; ++i)
			{
				float x = i * tilesize;
				float z = j * tilesize;
				vertices.push_back(x );
				vertices.push_back(0);
				vertices.push_back(z);

				vertices.push_back(0);
				vertices.push_back(1.0f);
				vertices.push_back(0);

				vertices.push_back((float)i / (float)resolution);
				vertices.push_back((float)j / (float)resolution);

				vertices.push_back(1.0f);
				vertices.push_back(0);
				vertices.push_back(0);

				vertices.push_back(0);
				vertices.push_back(0);
				vertices.push_back(1.0f);

			}
		}


		for (int j = 0; j < resolution; ++j)
		{
			for (int i = 0; i < resolution; ++i)
			{
				unsigned int p1 = j * (resolution + 1) + i;
				unsigned int p2 = p1 + (resolution + 1);

				indices.push_back(p1);
				indices.push_back(p2);
				indices.push_back(p2 + 1);

				indices.push_back(p1 );
				indices.push_back(p2 + 1);
				indices.push_back(p1 + 1);
			}
		}
		aabb.min.y = -0.1;
		aabb.max.y = 0.1;
	}
	else if (geom->second == "room" || geom->second == "cube")
	{
		int size = 10;
		if (params.find("size") != end)
		{
			size = atoi(params.find("size")->second.c_str());
		}
		float half = size *0.5f;
		float sign = 1.0f;
		if (geom->second == "room")
		{
			sign = -1.0f;
			for (int i = 0; i < 8; ++i)
			{
				indices.push_back(0 + i * 4);
				indices.push_back(3 + i * 4);
				indices.push_back(1 + i * 4);
				indices.push_back(0 + i * 4);
				indices.push_back(2 + i * 4);
				indices.push_back(3 + i * 4);
			}
		}
		else
		{
			for (int i = 0; i < 8; ++i)
			{
				indices.push_back(0 + i * 4);
				indices.push_back(1 + i * 4);
				indices.push_back(3 + i * 4);
				indices.push_back(0 + i * 4);
				indices.push_back(3 + i * 4);
				indices.push_back(2 + i * 4);
			}
		}
		vertices = {
			// -z
			-half,  half, -half,	0,0,-sign,	0, 0,  1,0,0,	0,-1,0,
			 half,  half, -half,	0,0,-sign,	1, 0,  1,0,0,	0,-1,0,
			-half, -half, -half,	0,0,-sign,	0, 1,  1,0,0,	0,-1,0,
			 half, -half, -half,	0,0,-sign,	1, 1,  1,0,0,	0,-1,0,
			// z
			 half,  half,  half,	0,0, sign,	0, 0,	-1,0,0,	0,-1,0,
			-half,  half,  half,	0,0, sign,	1, 0,	-1,0,0,	0,-1,0,
			 half, -half,  half,	0,0, sign,	0, 1,	-1,0,0,	0,-1,0,
			-half, -half,  half,	0,0, sign,	1, 1,	-1,0,0,	0,-1,0,
			// -x
			-half,  half,  half,	-sign,0, 0,	0, 0,	0,0,-1,	0,-1,0,
			-half,  half, -half,	-sign,0, 0,	1, 0,	0,0,-1,	0,-1,0,
			-half, -half,  half,	-sign,0, 0,	0, 1,	0,0,-1,	0,-1,0,
			-half, -half, -half,	-sign,0, 0,	1, 1,	0,0,-1,	0,-1,0,
			// x
			 half,  half, -half,	sign,0, 0,	0, 0,	0,0,1,	0,-1,0,
			 half,  half,  half,	sign,0, 0,	1, 0,	0,0,1,	0,-1,0,
			 half, -half, -half,	sign,0, 0,	0, 1,	0,0,1,	0,-1,0,
			 half, -half,  half,	sign,0, 0,	1, 1,	0,0,1,	0,-1,0,

			 // y
			 -half,  half,  half,	0,sign, 0,	0, 0,	1,0,0,	0,0,-1,
			  half,  half,  half,	0,sign, 0,	1, 0,	1,0,0,	0,0,-1,
			 -half,  half, -half,	0,sign, 0,	0, 1,	1,0,0,	0,0,-1,
			  half,  half, -half,	0,sign, 0,	1, 1,	1,0,0,	0,0,-1,
			 // -y
			  half, -half,  half,	0,-sign, 0,	0, 0,	-1,0,0,	0,0,-1,
			 -half, -half,  half,	0,-sign, 0,	1, 0,	-1,0,0,	0,0,-1,
			  half, -half, -half,	0,-sign, 0,	0, 1,	-1,0,0,	0,0,-1,
			 -half, -half, -half,	0,-sign, 0,	1, 1,	-1,0,0,	0,0,-1,
		};




		trans = DirectX::SimpleMath::Matrix::CreateFromAxisAngle({ 1,0,0 },0);
	}
	else
		abort();

	size_t vertex_stride = 3 + 3 + 2 + 3 + 3;
	size_t numVertices = vertices.size() / vertex_stride;
	for (size_t i = 0; i < numVertices; ++i)
	{
		auto x = vertices[0 + i * vertex_stride];
		auto y = vertices[1 + i * vertex_stride];
		auto z = vertices[2 + i * vertex_stride];
		Vector3 pos = Vector3::Transform({ x,y,z }, trans);
		aabb.min = DirectX::SimpleMath::Vector3::Min(aabb.min, pos);
		aabb.max = DirectX::SimpleMath::Vector3::Max(aabb.max, pos);
	}
	D3D11_SUBRESOURCE_DATA InitQuadData;
	ZeroMemory(&InitQuadData, sizeof(InitQuadData));
	InitQuadData.pSysMem = vertices.data();
	InitQuadData.SysMemPitch = 0;
	InitQuadData.SysMemSlicePitch = 0;
	auto vb = r->createBuffer(vertices.size() * sizeof(float), D3D11_BIND_VERTEX_BUFFER, &InitQuadData);

	InitQuadData.pSysMem = indices.data();
	auto ib = r->createBuffer(indices.size() * sizeof(int), D3D11_BIND_INDEX_BUFFER, &InitQuadData);

	std::vector<D3D11_INPUT_ELEMENT_DESC> layout = { 
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	auto lo = r->createLayout(layout.data(), layout.size());

	Mesh::Meshs meshs;
	auto material = Material::create();
	meshs.push_back({
		vb,
		ib,
		numVertices,
		indices.size(),
		material,
		lo,
		trans,
	});

	return { meshs, aabb };
}
