#pragma once

#include "Pipeline.h"
#include "GPUComputer.h"
class DepthBounding : public Pipeline::Stage
{
public:
	DepthBounding(
		Renderer::Ptr r,
		Scene::Ptr s,
		Setting::Ptr set,
		Pipeline* p ,
		Renderer::ShaderResource::Ptr depth);
	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::Texture::Ptr mDepthMinMax;
	Renderer::ShaderResource::Ptr mDepth;
	size_t mWidth;
	size_t mHeight;
};