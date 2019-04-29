#pragma once

#include <memory>
#include <functional>
#include "renderer.h"



class Material
{
public:
	using Ptr = std::shared_ptr<Material>;
public:
	Material();
	~Material();

	void setTexture(size_t index, Renderer::Texture::Ptr tex);
	
public:
	std::vector<Renderer::Texture::Ptr> mTextures;
	
	Renderer::Effect::Ptr mEffect;
};
