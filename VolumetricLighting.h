#pragma once

#include "Pipeline.h"

class VolumetricLighting:public Pipeline::Stage
{
public:
	VolumetricLighting(Renderer::Ptr r, Scene::Ptr s,Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~VolumetricLighting();

	void render(Renderer::Texture::Ptr rt)  override final;

private:
	void renderBlur(Renderer::Texture::Ptr rt);
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mColorFilter;

	Renderer::Texture::Ptr mBlur;
	Quad mQuad;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Sampler::Ptr mLinearClamp;
};