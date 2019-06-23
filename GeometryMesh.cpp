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
	size_t vertex_stride = 3 + 3 + 2 + 3 + 3;

	auto calTB = [&](UINT p1, UINT p2, UINT p3)
	{
		Vector3 v1 = { vertices[p1 * vertex_stride] , vertices[p1 * vertex_stride + 1], vertices[p1 * vertex_stride + 2] };
		Vector3 v2 = { vertices[p2 * vertex_stride] , vertices[p2 * vertex_stride + 1], vertices[p2 * vertex_stride + 2] };
		Vector3 v3 = { vertices[p3 * vertex_stride] , vertices[p3 * vertex_stride + 1], vertices[p3 * vertex_stride + 2] };

		Vector2 t1 = { vertices[p1 * vertex_stride + 6] , vertices[p1 * vertex_stride + 7] };
		Vector2 t2 = { vertices[p2 * vertex_stride + 6] , vertices[p2 * vertex_stride + 7] };
		Vector2 t3 = { vertices[p3 * vertex_stride + 6] , vertices[p3 * vertex_stride + 7] };



		auto calOne = [](
			const Vector3& v1, const Vector3& v2, const Vector3& v3,
			const Vector2& t1, const Vector2& t2, const Vector2& t3,
			Vector3& tangent, Vector3& bitangent)
		{
			Vector3 e1 = v2 - v1;
			Vector3 e2 = v3 - v1;
			Vector2 dt1 = t2 - t1;
			Vector2 dt2 = t3 - t1;

			float f = 1.0f / (dt1.x * dt2.y - dt2.x * dt1.y);
			tangent.x = f * (dt2.y * e1.x - dt1.y * e2.x);
			tangent.y = f * (dt2.y * e1.y - dt1.y * e2.y);
			tangent.z = f * (dt2.y * e1.z - dt1.y * e2.z);
			tangent.Normalize();
			bitangent.x = f * (-dt2.x * e1.x + dt1.x * e2.x);
			bitangent.y = f * (-dt2.x * e1.y + dt1.x * e2.y);
			bitangent.z = f * (-dt2.x * e1.z + dt1.x * e2.z);
			bitangent.Normalize();
		};


		Vector3 T1, B1;
		calOne(v1, v2, v3, t1, t2, t3, T1, B1);
		memcpy(vertices.data() + p1 * vertex_stride + 8, &T1, sizeof(Vector3));
		memcpy(vertices.data() + p1 * vertex_stride + 11, &B1, sizeof(Vector3));

		Vector3 T2, B2;
		calOne(v2, v1, v3, t2, t1, t3, T2, B2);
		memcpy(vertices.data() + p2 * vertex_stride + 8, &T2, sizeof(Vector3));
		memcpy(vertices.data() + p2 * vertex_stride + 11, &B2, sizeof(Vector3));

		Vector3 T3, B3;
		calOne(v3, v1, v2, t3, t1, t2, T3, B3);
		memcpy(vertices.data() + p3 * vertex_stride + 8, &T3, sizeof(Vector3));
		memcpy(vertices.data() + p3 * vertex_stride + 11, &B3, sizeof(Vector3));

	};


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
				unsigned int p3 = p1 + 1;
				unsigned int p4 = p2 + 1;
				indices.push_back(p1);
				indices.push_back(p2);
				indices.push_back(p3);


				indices.push_back(p3);
				indices.push_back(p2);
				indices.push_back(p4);

			}
		}

	}
	else if (geom->second == "plane")
	{
		float size = 10;
		int resolution = 1;

		if (params.find("size") != end)
		{
			std::stringstream ss;
			ss << params.find("size")->second;
			ss >> size;
		}

		//if (params.find("resolution") != end)
		//{
		//	resolution = atoi(params.find("resolution")->second.c_str());
		//}

		float half = size*0.5f;
		vertices = {
			-half, 0, -half, 0,1,0, 0,0, 1,0,0, 0,0,1,
			-half, 0, half, 0,1,0, 0,1, 1,0,0, 0,0,1,
			half, 0, half, 0,1,0, 1,1, 1,0,0, 0,0,1,
			half, 0, -half, 0,1,0, 1,0, 1,0,0, 0,0,1,
		};

		indices = {
			0,1,2,
			0,2,3
		};

		
		aabb.min.y = -half;
		aabb.max.y = half;
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
			sign = -1.0f;

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
			  -half,  half,  half,	0,sign, 0,	0, 0,	1,0,0,	0,0,1,
			   half,  half,  half,	0,sign, 0,	1, 0,	1,0,0,	0,0,1,
			  -half,  half, -half,	0,sign, 0,	0, 1,	1,0,0,	0,0,1,
			   half,  half, -half,	0,sign, 0,	1, 1,	1,0,0,	0,0,1,
			   // -y
				half, -half,  half,	0,-sign, 0,	0, 0,	-1,0,0,	0,0,-1,
			   -half, -half,  half,	0,-sign, 0,	1, 0,	-1,0,0,	0,0,-1,
				half, -half, -half,	0,-sign, 0,	0, 1,	-1,0,0,	0,0,-1,
			   -half, -half, -half,	0,-sign, 0,	1, 1,	-1,0,0,	0,0,-1,
		};

		
		if (geom->second == "room")
		{
			for (int i = 0; i < 6; ++i)
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
			for (int i = 0; i < 6; ++i)
			{
				indices.push_back(0 + i * 4);
				indices.push_back(1 + i * 4);
				indices.push_back(3 + i * 4);
				indices.push_back(0 + i * 4);
				indices.push_back(3 + i * 4);
				indices.push_back(2 + i * 4);

			}
		}


		trans = DirectX::SimpleMath::Matrix::CreateFromAxisAngle({ 1,0,0 },0);
	}
	else
		abort();

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
