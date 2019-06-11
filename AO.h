#pragma once


#include "Pipeline.h"
#include "ImageProcessing.h"

class AO :public Pipeline::Stage
{
	__declspec(align(16))
	struct Kernel
	{
		DirectX::XMFLOAT4 kernel[64];
		DirectX::XMFLOAT2 scale;
		int kernelSize;
	};

	__declspec(align(16))
	struct ConstantMatrix
	{
		DirectX::XMFLOAT4X4A invertProjection;
		DirectX::XMFLOAT4X4A projection;
		DirectX::XMFLOAT4X4A view;
		float radius;
		float intensity;
	};
public:
	AO(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set,Pipeline* p);
	
	void render(Renderer::Texture2D::Ptr rt) override;
	void init(float radius);
private:
	Renderer::Texture2D::Ptr mNoise;
	Renderer::ShaderResource::Ptr mNormal;
	Renderer::ShaderResource::Ptr mDepth;

	Renderer::PixelShader::Weak mPS;

	Renderer::Buffer::Ptr mKernel;
	Renderer::Buffer::Ptr mMatrix;

	Renderer::Sampler::Ptr mLinearWrap;
	Renderer::Sampler::Ptr mPointWrap;
	Gaussian::Ptr mGaussianFilter;
	Renderer::Texture2D::Ptr mAO;
};