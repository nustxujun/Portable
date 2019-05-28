#pragma once

#include "Pipeline.h"

class DepthLinearing :public Pipeline::Stage
{
public:
	DepthLinearing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline* p,
		Renderer::DepthStencil::Ptr depth, Renderer::Texture::Ptr depthlinear);

	void render(Renderer::Texture::Ptr rt)  override final;

private:
	Renderer::DepthStencil::Ptr mDepth;
	Renderer::Texture::Ptr mDepthLinear;
	Renderer::Buffer::Ptr mConstants;
	Renderer::PixelShader::Weak mPS;
};
