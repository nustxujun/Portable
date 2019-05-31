#pragma once

#include "Pipeline.h"

class DepthLinearing :public Pipeline::Stage
{
public:
	DepthLinearing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline* p);

	void render(Renderer::Texture2D::Ptr rt)  override final;

private:
	Renderer::ShaderResource::Ptr mDepth;
	Renderer::RenderTarget::Ptr mDepthLinear;
	Renderer::Buffer::Ptr mConstants;
	Renderer::PixelShader::Weak mPS;
};
