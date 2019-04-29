#pragma once
#include <vector>
#include <string>
class MeshBuilder
{
public:
	enum VertexFormat
	{
		POSITION,
		NORMAL,
		TEXCOORD0,
	};

	struct Data
	{
		struct Mesh {
			std::vector<VertexFormat> layout;
			std::vector<size_t> indices;
			std::vector<char> vertices;
			size_t numVertex;
			size_t materialIndex;
		};

		struct Material {
			std::vector<std::string> texture_diffuses;
		};

		std::vector<Mesh> meshs;
		std::vector<Material> materials;
		float aabb[6];
	};
public:
	static Data build(const std::string& filename);
};