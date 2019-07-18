#pragma once


#include "Mesh.h"

class VoxelMesh: public Mesh
{
public:
	using Buffer = std::unique_ptr<std::vector<char>>;

	VoxelMesh(const Parameters& params, Renderer::Ptr r);

	void load(size_t size,float scale, Buffer color, Buffer normal, Buffer material);
private:
	Renderer::Ptr mRenderer;
};

