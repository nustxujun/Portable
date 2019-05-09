#pragma once


#include "Pipeline.h"

class AO :public Pipeline::Stage
{
	struct Kernel
	{
		int kernelSize;
		float radius;
		DirectX::XMFLOAT2A scale;
		DirectX::XMFLOAT3A kernel[64];
	};

	struct Matrix
	{
		DirectX::XMFLOAT4X4A invertProjection;
		DirectX::XMFLOAT4X4A projection;
	};
public:
	AO(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);
	
	void render(Renderer::RenderTarget::Ptr rt) override;
private:
	Renderer::Texture::Ptr mNoise;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::RenderTarget::Ptr mDepth;

	Renderer::PixelShader::Weak mPS;

	Renderer::Buffer::Ptr mKernel;
	Renderer::Buffer::Ptr mMatrix;

	Renderer::Sampler::Ptr mLinearWrap;
};