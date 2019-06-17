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

	Renderer::Effect::Ptr getEffect(Material::Ptr mat);
private:

	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;


	std::unordered_map<size_t, Renderer::Effect::Ptr> mEffect;
};