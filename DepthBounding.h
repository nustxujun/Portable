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
		Renderer::ShaderResource::Ptr depth,
		Renderer::Texture::Ptr db);
	void render(Renderer::Texture::Ptr rt)  override final;
private:
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::Texture::Ptr mDepthMinMax;
	Renderer::ShaderResource::Ptr mDepth;
	Renderer::Buffer::Ptr mConstants;
};