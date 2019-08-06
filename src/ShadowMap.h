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
		Vector3 lightdir;
		int numcascades;
		float scale;
		float shadowcolor;
		float depthbias;
		float translucency;
		float thickness;
		float translucency_bias;
	};
public:
	ShadowMap(Renderer::Ptr r, Scene::Ptr s,Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~ShadowMap();


	void init(int mapsize, int numlevels, const std::vector<Renderer::Texture2D::Ptr>& rts, bool transmittance = false);
	void render(Renderer::Texture2D::Ptr rt) ;
private:
	void renderToShadowMapDir(const Matrix& lightview, const Scene::Light::Cascades& cascades, Renderer::Texture2D::Ptr tex);
	void renderShadowDir(const Matrix& lightview, const Scene::Light::Cascades& cascades, const Vector3& dir, Renderer::Texture2D::Ptr depth, Renderer::RenderTarget::Ptr rt);

	void renderToShadowMapPoint(Scene::Light::Ptr light, Renderer::Texture2D::Ptr tex);
	void renderShadowPoint(const Vector3& lightpos, Renderer::Texture2D::Ptr depth, Renderer::RenderTarget::Ptr rt);
private:
	bool mExponential = true;
	int mNumLevels;
	int mNumMaps;


	std::map<Scene::Light::Ptr,Renderer::Texture2D::Ptr> mShadowMaps;
	Renderer::Texture2D::Ptr mDefaultDS;
	Renderer::Texture2D::Ptr mDefaultCascadeDS;

	std::vector<Renderer::Texture2D::Ptr> mShadowTextures;
	int mShadowMapSize = 2048;
	Renderer::Buffer::Ptr mCastConstants;
	Renderer::Buffer::Ptr mReceiveConstants;

	Renderer::Effect::Ptr mShadowMapEffect;
	Renderer::Layout::Ptr mDepthLayout;
	Renderer::PixelShader::Weak mReceiveShadowPS[3];
	D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;
	Renderer::Sampler::Ptr mShadowSampler;
	Renderer::VertexShader::Weak mShadowVS;
	Renderer::PixelShader::Weak mShadowPS;
	Renderer::PixelShader::Weak mPointLightCast;
	Renderer::Buffer::Ptr mPointCastPSConsts;
	Renderer::Rasterizer::Ptr mRasterizer; 
	Renderer::ShaderResource::Ptr mSceneDepth;

	Quad mQuad;

	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
};