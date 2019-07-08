#pragma once
#include <vector>
#include <string>
#include <array>
class MeshBuilder
{
public:
	enum VertexFormat
	{
		POSITION,
		NORMAL,
		TEXCOORD0,
		TANGENT,
		BITANGENT,
	};

	struct Data
	{
		struct Mesh {
			std::vector<VertexFormat> layout;
			std::vector<unsigned int> indices;
			std::vector<char> vertices;
			size_t numVertex;
			size_t materialIndex;
			float tranfromation[16];
		};

		struct Material {
			std::string albedo;
			std::string normal;
			std::string ambient;
			std::string height;
			std::string shininess;

			std::array<float, 3> diffuse = {1,1,1};
		};

		std::vector<Mesh> meshs;
		std::vector<Material> materials;
		std::array<float,6> aabb;
	};
public:
	static Data build(const std::string& filename);

private:
	static Data buildByAssimp(const std::string& filename);
	static Data buildByTinyobj(const std::string& filename);
};