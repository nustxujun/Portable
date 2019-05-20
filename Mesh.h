#pragma once
#include "Common.h"
#include "renderer.h"
#include "Material.h"
#include "SimpleMath.h"
struct Renderable
{
	Renderer::Buffer::Ptr vertices;
	Renderer::Buffer::Ptr indices;
	size_t numVertices = 0;
	size_t numIndices = 0;
	Material::Ptr material;
	Renderer::Layout::Ptr layout;
	DirectX::SimpleMath::Matrix tranformation;
};

class Mesh
{
public:
	using Meshs = std::vector<Renderable>;
	using Ptr = std::shared_ptr<Mesh>;
	struct AABB
	{
		DirectX::SimpleMath::Vector3 min;
		DirectX::SimpleMath::Vector3 max;
	};
protected:
	Mesh(std::pair<Meshs, AABB >&&);

public:
	Mesh(const Parameters& params, Renderer::Ptr r);

	~Mesh();

	size_t getNumMesh() { return mMeshs.size(); }
	const Renderable& getMesh(size_t index) { return mMeshs[index]; }
	const AABB & getAABB() const{ return mAABB; }

	void setMaterial(Material::Ptr m);
private:
	AABB mAABB;
	Meshs mMeshs;
};