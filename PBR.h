#pragma once

#include "Pipeline.h"
#include "Quad.h"
class PBR:public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertViewPorj;
		Vector4 lightDir;
		Vector4 cameraPos;
		float radiance;
		float roughness;
		float metallic;
	};
public:
	PBR(
		Renderer::Ptr r,
		Scene::Ptr s,
		Pipeline* p,
		Renderer::RenderTarget::Ptr a,
		Renderer::RenderTarget::Ptr n,
		Renderer::DepthStencil::Ptr d, 
		float roughness,
		float metallic,
		float radiance);
	~PBR();

	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;
	float mRoughness;
	float mMetallic;
	float mRadiance;

	Renderer::PixelShader::Weak mPS;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
	Renderer::Buffer::Ptr mConstants;
	Quad mQuad;
};