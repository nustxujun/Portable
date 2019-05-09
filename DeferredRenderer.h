#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Pipeline.h"

class DeferredRenderer: public Pipeline::Stage
{
	struct LightningConstantBuffer
	{
		DirectX::XMFLOAT3A mLightDirection;
		DirectX::XMFLOAT4A mColor;
		DirectX::XMFLOAT3A mCameraPosition;
		DirectX::XMFLOAT4X4A mInvertViewMatrix;
	};
public:
	DeferredRenderer(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);

	void render(Renderer::RenderTarget::Ptr rt);
private:
	void renderGbuffer();
	void renderLightingMap();
	void renderFinal();
private:
	Renderer::Ptr mRenderer;

	Renderer::Effect::Ptr mSceneFX;
	Renderer::Layout::Ptr mGBufferLayout;
	Renderer::RenderTarget::Ptr mDiffuse;
	Renderer::RenderTarget::Ptr mNormal;
	Renderer::RenderTarget::Ptr mDepth;

	Renderer::PixelShader::Weak mLightingPS;
	Renderer::RenderTarget::Ptr mLightingMap;
	Renderer::Buffer::Ptr mLightingConstants;
	
	Renderer::Sampler::Ptr mSampleLinear;
	Renderer::Sampler::Ptr mSamplePoint;

	Renderer::PixelShader::Weak mFinalPS;
	Renderer::RenderTarget::Ptr mFinalTarget;


	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	D3D11_BLEND_DESC mDefaultBlendState;
};