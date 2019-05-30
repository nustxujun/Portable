#pragma once


#include "Pipeline.h"
#include "GPUComputer.h"
class LightCulling: public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertProj;
		int numLights;
		float texelwidth;
		float texelheight;
		int maxLightsPerTile;
		int tilePerline;
	};
public:
	LightCulling(
		Renderer::Ptr r,
		Scene::Ptr s,
		Quad::Ptr q,
		Setting::Ptr set,
		Pipeline* p,
		int w, 
		int h
	);
	void render(Renderer::Texture::Ptr rt)  override final;
private:
	Renderer::ShaderResource::Ptr mDepthBounds;
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::Buffer::Ptr mConstants;
	Renderer::ShaderResource::Ptr mLights;
	Renderer::UnorderedAccess::Ptr mLightsOutput;
	int mWidth;
	int mHeight;
};