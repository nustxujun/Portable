#pragma once

#include "Renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Pipeline.h"

class ShadowMap: public Pipeline::Stage
{
	struct ConstantMatrix
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 proj;
	};
public:
	ShadowMap(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);
	~ShadowMap();

	void render(Renderer::RenderTarget::Ptr rt);
private:
	void fitToScene();
	void renderToShadowMap();
	void renderShadow();


private:
	Renderer::RenderTarget::Ptr mFinalTarget;
	int mNumLevels;
	std::vector<DirectX::SimpleMath::Matrix> mProjections;
	std::vector<Vector4> mCascadeDepths;
	Renderer::DepthStencil::Ptr mShadowMap;
	Scene::Camera::Ptr mLightCamera;

	int mShadowMapSize = 2048;
	Matrix mLightView;
	Renderer::Buffer::Ptr mConstants;

	Renderer::Layout::Ptr mDepthLayout;
	Renderer::Effect::Ptr mDepthEffect;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	Renderer::Sampler::Ptr mSampler;
	Renderer::VertexShader::Weak mShadowVS;
	Renderer::Rasterizer::Ptr mRasterizer;

};