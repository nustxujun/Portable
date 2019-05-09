#pragma once

#include "Renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Pipeline.h"

class ShadowMap: public Pipeline::Stage
{
public:
	ShadowMap(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);
	~ShadowMap();

	void render(Renderer::RenderTarget::Ptr rt);
private:
	void fitToScene();
	void renderToLightMap();
	void renderShadow();
private:
	Renderer::RenderTarget::Ptr mFinalTarget;
	std::vector<Renderer::RenderTarget::Ptr > mLightMaps;
	Scene::Camera::Ptr mLightCamera;

	int mShadowMapSize = 2048;

	Renderer::Layout::Ptr mDepthLayout;
	Renderer::Effect::Ptr mDepthEffect;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	Renderer::DepthStencil::Ptr mDepthStencil;
	Renderer::Sampler::Ptr mSampler;
};