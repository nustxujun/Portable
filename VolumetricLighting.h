#pragma once

#include "Pipeline.h"

class VolumetricLighting:public Pipeline::Stage
{
public:
	VolumetricLighting(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr set, Pipeline* p);
	~VolumetricLighting();

	void render(Renderer::RenderTarget::Ptr rt) override final;

private:
	void renderBlur(Renderer::RenderTarget::Ptr rt);
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mColorFilter;

	Renderer::RenderTarget::Ptr mBlur;
	Quad mQuad;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Sampler::Ptr mLinearClamp;
};