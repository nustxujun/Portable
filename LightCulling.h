#pragma once


#include "Pipeline.h"
#include "GPUComputer.h"
class LightCulling: public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertProj;
		int numPointLights;
		int numSpotLights;
		float texelwidth;
		float texelheight;
	};
public:
	LightCulling(
		Renderer::Ptr r,
		Scene::Ptr s,
		Quad::Ptr q,
		Setting::Ptr set,
		Pipeline* p
	);
	void render(Renderer::Texture2D::Ptr rt)  override final;
	void init(int w, int h) { mWidth = w; mHeight = h; }
private:
	Renderer::ShaderResource::Ptr mDepthBounds;
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::Buffer::Ptr mConstants;
	Renderer::UnorderedAccess::Ptr mLightsOutput;
	Renderer::Buffer::Ptr mCurIndex;

	int mWidth;
	int mHeight;
};