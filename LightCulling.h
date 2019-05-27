#pragma once


#include "Pipeline.h"
#include "GPUComputer.h"
class LightCulling: public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertProj;
		Vector4 lights[100];
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
		Renderer::Texture::Ptr depthBounds,
		Renderer::Buffer::Ptr lightsindex
	);
	void render(Renderer::Texture::Ptr rt)  override final;
private:
	Renderer::Texture::Ptr mDepthBounds;
	GPUComputer mComputer;
	Renderer::ComputeShader::Weak mCS;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Buffer::Ptr mLightsOutput;
};