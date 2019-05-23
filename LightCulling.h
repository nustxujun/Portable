#pragma once


#include "Pipeline.h"
#include "GPUComputer.h"
class LightCulling: public Pipeline::Stage
{
public:
	LightCulling(
		Renderer::Ptr r,
		Scene::Ptr s,
		Setting::Ptr set,
		Pipeline* p,
		Renderer::ShaderResource::Ptr depthBounds
	);
	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	Renderer::ShaderResource::Ptr mDepthBounds;
	Renderer::Texture::Ptr mOutput;
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	size_t mWidth;
	size_t mHeight;
};