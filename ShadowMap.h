#pragma once

#include "Renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Pipeline.h"

class ShadowMap: public Pipeline::Stage
{
	struct CastConstants
	{
		Matrix world;
		Matrix view;
		Matrix proj;
	};

	__declspec(align(16))
	struct ReceiveConstants
	{
		Matrix invertViewProj;
		Matrix lightView;
		Matrix lightProjs[8];
		Vector4 cascadeDepths[8];
		int numcascades;
		float scale;
	};
public:
	ShadowMap(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr set, Pipeline* p,Renderer::ShaderResource::Ptr worldpos, Renderer::ShaderResource::Ptr depth, int mapsize, int numlevels);
	~ShadowMap();

	void render(Renderer::RenderTarget::Ptr rt);
private:
	void fitToScene();
	void renderToShadowMap();
	void renderShadow(Renderer::RenderTarget::Ptr rt);


private:
	int mNumLevels;
	std::vector<DirectX::SimpleMath::Matrix> mProjections;
	std::vector<Vector4> mCascadeDepths;
	Renderer::DepthStencil::Ptr mShadowMap;

	int mShadowMapSize = 2048;
	Matrix mLightView;
	Renderer::Buffer::Ptr mCastConstants;
	Renderer::Buffer::Ptr mReceiveConstants;


	Renderer::Layout::Ptr mDepthLayout;
	Renderer::Effect::Ptr mDepthEffect;
	Renderer::PixelShader::Weak mReceiveShadowPS;
	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	Renderer::Sampler::Ptr mSampler;
	Renderer::VertexShader::Weak mShadowVS;
	Renderer::Rasterizer::Ptr mRasterizer;
	Renderer::ShaderResource::Ptr mSceneDepth;
	Renderer::ShaderResource::Ptr mWorldPos;

	Quad mQuad;

	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
};