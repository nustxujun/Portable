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

};