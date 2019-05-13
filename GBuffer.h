#pragma once


#include "Pipeline.h"

class GBuffer:public Pipeline::Stage
{
public:
	GBuffer(
		Renderer::Ptr r, 
		Scene::Ptr s, 
		Pipeline* p,
		Renderer::RenderTarget::Ptr a, 
		Renderer::RenderTarget::Ptr n, 
		Renderer::DepthStencil::Ptr d);
	~GBuffer();


	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;

	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
};