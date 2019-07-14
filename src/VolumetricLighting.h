#pragma once

#include "Pipeline.h"

class VolumetricLighting:public Pipeline::Stage
{
public:
	VolumetricLighting(Renderer::Ptr r, Scene::Ptr s,Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~VolumetricLighting();

	void render(Renderer::Texture2D::Ptr rt)  override final;

private:
	void renderBlur(Renderer::Texture2D::Ptr rt);
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mColorFilter;

	Renderer::Texture2D::Ptr mBlur;
	Quad mQuad;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Sampler::Ptr mLinearClamp;
};