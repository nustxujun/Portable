#pragma once


#include "Mesh.h"

class GeometryMesh: public Mesh
{
public:
	GeometryMesh(const Parameters& params, Renderer::Ptr r);
	~GeometryMesh();

private:
	static std::pair<Mesh::Meshs , Mesh::AABB> generateGeometry(const Parameters& params, Renderer::Ptr r);
};

