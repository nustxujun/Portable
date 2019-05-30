#pragma once

#include "Pipeline.h"
#include "GPUComputer.h"
class DepthBounding : public Pipeline::Stage
{
public:
	DepthBounding(
		Renderer::Ptr r,
		Scene::Ptr s,
		Quad::Ptr q,
		Setting::Ptr set,
		Pipeline* p );
	void render(Renderer::Texture::Ptr rt)  override final;
	void init(int width, int height) ;
private:
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::UnorderedAccess::Ptr mDepthMinMax;
	Renderer::ShaderResource::Ptr mDepth;
	Renderer::Buffer::Ptr mConstants;
	int mWidth;
	int mHeight;
};