#pragma once

#include "Pipeline.h"
#include "GPUComputer.h"
class DepthBounding : public Pipeline::Stage
{
public:
	DepthBounding(
		Renderer::Ptr r,
		Scene::Ptr s,
		Pipeline* p );
	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
};