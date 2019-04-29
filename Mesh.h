#pragma once
#include "Common.h"
#include "renderer.h"
#include "Material.h"
struct Renderable
{
	Renderer::Buffer::Ptr vertices;
	Renderer::Buffer::Ptr indices;
	size_t numVertices = 0;
	size_t numIndices = 0;
	Material::Ptr material;
	size_t strideVertex = 0;

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