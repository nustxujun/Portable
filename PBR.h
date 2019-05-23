#pragma once

#include "Pipeline.h"
#include "Quad.h"
class PBR:public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertPorj;
		Matrix View;
		Vector4 lightpos;
		Vector3 radiance;
		float roughness;
		float metallic;
	};
public:
	PBR(
		Renderer::Ptr r,
		Scene::Ptr s,
		Setting::Ptr set,
		Pipeline* p,
		Renderer::RenderTarget::Ptr a,
		Renderer::RenderTarget::Ptr n,
		Renderer::DepthStencil::Ptr d, 
		float roughness,
		float metallic);
	~PBR();

	void render(Renderer::RenderTarget::Ptr rt) override final;
private:
	Renderer::RenderTarget::Ptr mAlbedo;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;
	float mRoughness;
	float mMetallic;

	std::array<Renderer::PixelShader::Weak,3> mPSs;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
	Renderer::Buffer::Ptr mConstants;
	Quad mQuad;
};