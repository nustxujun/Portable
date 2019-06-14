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
public:
	Material();
	~Material();

	void setTexture(size_t index, Renderer::Texture2D::Ptr tex);
	bool hasTexture() { return !mTextures.empty(); }

	void enableLighting(bool l) { mLighting = l; }
	bool isLightingEnabled()const { return mLighting; }
public:
	std::vector<Renderer::Texture2D::Ptr> mTextures;
	bool mLighting = true;
	float metallic = 0.5f;
	float roughness = 0.5f;
};
