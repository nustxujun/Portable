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


	void render(Renderer::Texture2D::Ptr rt)  override final;
private:
	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;

	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
};