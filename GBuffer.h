#pragma once


#include "Pipeline.h"

class GBuffer:public Pipeline::Stage
{
public:
	GBuffer(
		Renderer::Ptr r, 
		Scene::Ptr s, 
		Quad::Ptr q,
		Setting::Ptr set,
		Pipeline* p);
	~GBuffer();


	void render(Renderer::Texture::Ptr rt)  override final;
private:
	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::RenderTarget::Ptr mWorldPos;
	Renderer::DepthStencil::Ptr mDepth;

	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
};