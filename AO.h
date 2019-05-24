#pragma once


#include "Pipeline.h"

class AO :public Pipeline::Stage
{
	struct Kernel
	{
		int kernelSize;
		float radius;
		DirectX::XMFLOAT2 scale;
		DirectX::XMFLOAT3A kernel[64];
	};

	struct ConstantMatrix
	{
		DirectX::XMFLOAT4X4A invertProjection;
		DirectX::XMFLOAT4X4A projection;
		DirectX::XMFLOAT4X4A view;
	};
public:
	AO(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr set,Pipeline* p, Renderer::ShaderResource::Ptr normal, Renderer::ShaderResource::Ptr depth, float radius);
	
	void render(Renderer::Texture::Ptr rt) override;
private:
	Renderer::Texture::Ptr mNoise;
	Renderer::ShaderResource::Ptr mNormal;
	Renderer::ShaderResource::Ptr mDepth;

	Renderer::PixelShader::Weak mPS;

	Renderer::Buffer::Ptr mKernel;
	Renderer::Buffer::Ptr mMatrix;

	Renderer::Sampler::Ptr mLinearWrap;
	Renderer::Sampler::Ptr mPointWrap;

};