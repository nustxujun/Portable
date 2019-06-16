#pragma once
#include "Pipeline.h"
#include "ImageProcessing.h"
class HDR :public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		float keyvalue;
		float lumMin;
		float lumMax;
	};
public:
	HDR(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~HDR();
	void render(Renderer::Texture2D::Ptr rt) override;
	void init();
private:
	void renderLuminance(Renderer::Texture2D::Ptr rt);
	void renderHDR(Renderer::Texture2D::Ptr frame);
	void renderBrightness(Renderer::Texture2D::Ptr rt);
	void renderBloom(Renderer::Texture2D::Ptr rt);
private:
	Renderer::Texture2D::Ptr mTarget;
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mDownSamplePS2x2;
	Renderer::PixelShader::Weak mDownSamplePS3x3;
	Renderer::Sampler::Ptr mPoint;

	std::vector<Renderer::Texture2D::Ptr> mLuminance;
	Renderer::Texture2D::Ptr mExposure;
	Renderer::PixelShader::Weak mCalExposure;
	Renderer::Buffer::Ptr mConstants;

	Renderer::Texture2D::Ptr mBloomRT;
	Renderer::PixelShader::Weak mBrightPass;
	Renderer::PixelShader::Weak mGaussianBlur[2];
	Renderer::Buffer::Ptr mBloomConstants;
	SamplingBox::Ptr mDownsample;
	Gaussian::Ptr mGaussian;

};