#include "Material.h"

Material::Material()
{
}

Material::~Material()
{
}

void Material::setTexture(size_t index, Renderer::Texture::Ptr tex)
{
	if (mTextures.size() <= index)
		mTextures.resize(index + 1);

	mTextures[index] = tex;
}

