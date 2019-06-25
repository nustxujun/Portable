#pragma once


#include "Pipeline.h"
#include "ImageProcessing.h"
class SSR :public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix view;
		Matrix proj;
		Matrix invertProj;
		float reflection;
		float raylength;
		float width;
		float height;
		float stepstride;
		float stridescale;
		float nearZ;
		float jitter;
	};

	struct MatrixConstants
	{
		Matrix world;
		Matrix view;
		Matrix proj;
	};
public:
	using Pipeline::Stage::Stage;
	void init();
	void render(Renderer::Texture2D::Ptr rt) override;
private:
	void renderDepthBack();
	void renderRaytrace(Renderer::Texture2D::Ptr rt);
	void renderColor(Renderer::Texture2D::Ptr rt);
private:
	Renderer::PixelShader::Weak mRayTracing;
	Renderer::PixelShader::Weak mLighting;

	Renderer::VertexShader::Weak mVS;
	Renderer::Texture2D::Ptr mHitmap;
	Renderer::Texture2D::Ptr mFrame;
	Renderer::Texture2D::Ptr mBlueNoise;


	Renderer::Texture2D::Ptr mDepthBack;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Buffer::Ptr mMatrixConst;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;

	SamplingBox::Ptr mSample;
	Gaussian::Ptr mGaussian;
};