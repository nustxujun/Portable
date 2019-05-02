#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"

class DeferredRenderer
{
	struct LightningConstantBuffer
	{
		DirectX::XMFLOAT4A mLightDirection;
		DirectX::XMFLOAT4A mColor;
		DirectX::XMFLOAT3A mCameraPosition;
		DirectX::XMFLOAT4X4A mInvertViewMatrix;
	};
public:
	DeferredRenderer(Renderer::Ptr r, Scene::Ptr s);

	void render();
private:
	void renderGbuffer();
	void renderLightingMap();
	void renderFinal();
private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;

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

	std::shared_ptr<Quad> mQuad;
	Renderer::PixelShader::Weak mDrawTexturePS;


};