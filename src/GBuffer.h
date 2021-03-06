#pragma once


#include "Pipeline.h"

class GBuffer:public Pipeline::Stage
{
public:
	using Pipeline::Stage::Stage;
	~GBuffer();


	void render(Renderer::Texture2D::Ptr rt)  override final;
	void init(bool cleardepth, std::function<bool(Scene::Entity::Ptr)> cond = {});
private:

	Renderer::Effect::Ptr getEffect(Material::Ptr mat);
private:
	std::function<bool(Scene::Entity::Ptr)> mRenderCond;
	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;
	Renderer::Rasterizer::Ptr mRasterizer;
	bool mClearDepth;

	std::unordered_map<size_t, Renderer::Effect::Ptr> mEffect;
};