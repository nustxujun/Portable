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
		Renderer::UnorderedAccess::Ptr db);
	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::UnorderedAccess::Ptr mOutput;
	Renderer::ShaderResource::Ptr mDepth;
};