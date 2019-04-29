#pragma once
#include "renderer.h"
#include <string>
struct RenderOp
{
	Renderer::Buffer::Ptr vertices;
	Renderer::Buffer::Ptr indices;
	size_t numVertices = 0;
	size_t numIndices = 0;
	Renderer::Texture::Ptr texture;
};

class Mesh
{
public:
	using Meshs = std::vector<RenderOp>;
	using Ptr = std::shared_ptr<Mesh>;
	struct AABB
	{
		float min[3];
		float max[3];
	};
public:
	Mesh(const std::string& filename, Renderer::Ptr r);
	~Mesh();

	size_t getNumMesh() { return mMeshs.size(); }
	RenderOp getMesh(size_t index) { return mMeshs[index]; }
private:

	AABB mAABB;
	Meshs mMeshs;
};