#pragma once

#include "Renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Pipeline.h"

class ShadowMap: public Pipeline::Stage
{
	static constexpr auto MAX_LEVELS = 8;
	static constexpr auto MAX_LIGHTS = 8;

	__declspec(align(16))
	struct CastConstants
	{
		Matrix world;
		Matrix view;
		Matrix proj;
	};

	__declspec(align(16))
	struct ReceiveConstants
	{
		Matrix invertView;
		Matrix invertProj;
		Matrix lightView;
		Matrix lightProjs[8];
		Vector4 cascadeDepths[8];
		int numcascades;
		float scale;
		float shadowcolor;
		float depthbias;
	};
public:
	ShadowMap(Renderer::Ptr r, Scene::Ptr s,Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~ShadowMap();


	void init(int mapsize, int numlevels, const std::vector<Renderer::Texture2D::Ptr>& rts);
	void render(Renderer::Texture2D::Ptr rt) ;
private:
	void fitToScene(int index, Scene::Light::Ptr l);
	void renderToShadowMap(int index);
	void renderShadow(int index, Renderer::RenderTarget::Ptr rt);


private:
	int mNumLevels;
	int mNumMaps;
	struct MapParams
	{
		std::array<DirectX::SimpleMath::Matrix, MAX_LEVELS> projs;
		std::array<float, MAX_LEVELS> depths;
	};
	std::vector<MapParams> mMapParams;

	std::vector<Renderer::Texture2D::Ptr> mShadowMaps;
	std::vector<Renderer::Texture2D::Ptr> mShadowTextures;
	int mShadowMapSize = 2048;
	Matrix mLightView;
	Renderer::Buffer::Ptr mCastConstants;
	Renderer::Buffer::Ptr mReceiveConstants;


	Renderer::Layout::Ptr mDepthLayout;
	Renderer::Effect::Ptr mDepthEffect;
	Renderer::PixelShader::Weak mReceiveShadowPS;
	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	Renderer::Sampler::Ptr mShadowSampler;
	Renderer::VertexShader::Weak mShadowVS;
	Renderer::PixelShader::Weak mShadowPS;
	Renderer::Rasterizer::Ptr mRasterizer;
	Renderer::ShaderResource::Ptr mSceneDepth;

	Quad mQuad;

	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
};