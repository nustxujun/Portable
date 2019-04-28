#pragma once
#include "renderer.h"
#include <functional>
#include "Scene.h"
class GBuffer
{
public:
	GBuffer(Renderer::Ptr r);
	~GBuffer();

	void render(Scene::Ptr scene);

	Renderer::RenderTarget::Ptr getDiffuse() { return mDiffuse; }
	Renderer::RenderTarget::Ptr getNormal() { return mNormal; }
	Renderer::RenderTarget::Ptr getDepth() { return mDepth; }

private:
	Renderer::Ptr mRenderer;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;

	Renderer::RenderTarget::Ptr mDiffuse;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::RenderTarget::Ptr mDepth;
};