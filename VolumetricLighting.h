#pragma once

#include "Pipeline.h"

class VolumetricLighting:public Pipeline::Stage
{
public:
	VolumetricLighting(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);
	~VolumetricLighting();

	void render(Renderer::RenderTarget::Ptr rt) override final;

private:
	void renderBlur(Renderer::RenderTarget::Ptr rt);
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::RenderTarget::Ptr mBlur;
	Quad mQuad;
	Renderer::Buffer::Ptr mConstants;
};