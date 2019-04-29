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
		};

		std::vector<Mesh> meshs;
		std::vector<std::string> textures;
		float aabb[6];
	};
public:
	static Data build(const std::string& filename);
};