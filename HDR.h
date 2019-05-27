#pragma once
#include "Pipeline.h"

class HDR :public Pipeline::Stage
{
public:
	HDR(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~HDR();
	void render(Renderer::Texture::Ptr rt) override;
private:
	void renderLuminance(Renderer::Texture::Ptr rt);
	void renderHDR(Renderer::Texture::Ptr frame);
private:
	Renderer::Texture::Ptr mTarget;
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mDownSamplePS2x2;
	Renderer::PixelShader::Weak mDownSamplePS3x3;
	Renderer::Sampler::Ptr mPoint;

	std::vector<Renderer::Texture::Ptr> mLuminance;
};