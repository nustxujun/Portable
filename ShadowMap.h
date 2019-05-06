#pragma once

#include "Renderer.h"
#include "Scene.h"
#include "Quad.h"

class ShadowMap
{
public:
	ShadowMap(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q);
	~ShadowMap();

	void render();
private:
	void fitToScene();
	void renderToLightMap();
	void renderShadow();
private:
	Renderer::RenderTarget::Ptr mFinalTarget;
	Renderer::RenderTarget::Ptr mLightMap;
	Scene::Ptr mScene;
	Renderer::Ptr mRenderer;
	Scene::Camera::Ptr mLightCamera;

	int mShadowMapSize = 2048;

	Renderer::Layout::Ptr mDepthLayout;
	Renderer::Effect::Ptr mDepthEffect;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	Renderer::DepthStencil::Ptr mDepthStencil;
	Renderer::Sampler::Ptr mSampler;
	Quad::Ptr mQuad;
};