#pragma once

#include <memory>
#include <functional>
#include "renderer.h"


class Material
{
public:
	using Ptr = std::shared_ptr<Material>;
	static Ptr create()
	{
		return Ptr(new Material());
	}

	enum TextureUnit
	{
		TU_ALBEDO,
		TU_NORMAL,
		TU_ROUGH,
		TU_METAL,
		TU_AO,
		TU_HEIGHT,
		TU_BUMP,

		TU_MAX
	};
public:
	Material();
	~Material();

	void setTexture(size_t index, Renderer::Texture2D::Ptr tex);
	bool hasTexture() { return !textures.empty(); }

	void enableLighting(bool l) { beLighting = l; }
	bool isLightingEnabled()const { return beLighting; }

	size_t getShaderID();
	std::vector<D3D10_SHADER_MACRO> generateShaderID();
public:
	std::vector<Renderer::Texture2D::Ptr> textures;
	bool beLighting = true;
	float metallic = 0.0f;
	float roughness = 1.0f;
	float reflection = 1.0f;
	DirectX::SimpleMath::Vector3 diffuse = { 1,1,1 };
	bool usingVertexColor = false;
private:
	size_t mShaderID = -1;


public:
	static  Material::Ptr Default;
};
