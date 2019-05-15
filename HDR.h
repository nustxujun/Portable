#pragma once
#include "Pipeline.h"

class HDR :public Pipeline::Stage
{
public:
	HDR(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);
	~HDR();
	void render(Renderer::RenderTarget::Ptr rt) override;
private:
	void renderLuminance(Renderer::RenderTarget::Ptr rt);
private:
	Renderer::RenderTarget::Ptr mTarget;
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mDownSamplePS2x2;
	Renderer::PixelShader::Weak mDownSamplePS3x3;
	Renderer::Sampler::Ptr mPoint;

	Quad mQuad;
	std::vector<Renderer::RenderTarget::Ptr> mLuminance;
};