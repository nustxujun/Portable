#pragma once
#include "Common.h"
#include "renderer.h"
#include "Material.h"
#include <D3DX10math.h>
struct Renderable
{
	Renderer::Buffer::Ptr vertices;
	Renderer::Buffer::Ptr indices;
	size_t numVertices = 0;
	size_t numIndices = 0;
	Material::Ptr material;
	Renderer::Layout::Ptr layout;
	D3DXMATRIX tranformation;
};

class Mesh
{
public:
	using Meshs = std::vector<Renderable>;
	using Ptr = std::shared_ptr<Mesh>;
	struct AABB
	{
		float min[3];
		float max[3];
	};
public:
	Mesh(const Parameters& params, Renderer::Ptr r);
	~Mesh();

	size_t getNumMesh() { return mMeshs.size(); }
	Renderable getMesh(size_t index) { return mMeshs[index]; }
private:
	AABB mAABB;
	Meshs mMeshs;
};