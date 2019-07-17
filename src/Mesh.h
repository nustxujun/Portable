#pragma once
#include "Common.h"
#include "renderer.h"
#include "Material.h"
#include "SimpleMath.h"


struct Renderable 
{
	size_t id = -1;
	Renderer::Buffer::Ptr vertices;
	Renderer::Buffer::Ptr indices;
	size_t numVertices = 0;
	size_t numIndices = 0;
	Material::Ptr material;
	Renderer::Layout::Ptr layout;
	DirectX::SimpleMath::Matrix tranformation;

	Renderable(const Renderable&) = default;
	Renderable(
		Renderer::Buffer::Ptr v,
		Renderer::Buffer::Ptr i,
		size_t nv,
		size_t ni,
		Material::Ptr m,
		Renderer::Layout::Ptr l,
		DirectX::SimpleMath::Matrix t):
		vertices(v), indices(i), numVertices(nv), numIndices(ni), material(m), layout(l),tranformation(t)
	{
		id = ID_GEN++;
	}
private:
	static size_t ID_GEN;
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
	Mesh() {};
public:
	Mesh(const Parameters& params, Renderer::Ptr r);

	~Mesh();

	size_t getNumMesh() { return mMeshs.size(); }
	const Renderable& getMesh(size_t index) { return mMeshs[index]; }
	const AABB & getAABB() const{ return mAABB; }

	void setMaterial(Material::Ptr m);
protected:
	AABB mAABB;
	Meshs mMeshs;
};