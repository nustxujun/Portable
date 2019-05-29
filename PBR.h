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
		Vector4 lightspos[100];
		Vector4 lightscolor[100];
		int numLights;
		float roughness;
		float metallic;
		float width;
		float height;
		int maxLightsPerTile;
		int tilePerline;
		float range;
	};

	__declspec(align(16))
	struct LightVolumeConstants
	{
		Matrix View;
		Matrix Projection;
	};
public:
	PBR(
		Renderer::Ptr r,
		Scene::Ptr s,
		Quad::Ptr q,
		Setting::Ptr set,
		Pipeline* p,
		Renderer::Texture::Ptr a,
		Renderer::Texture::Ptr n,
		Renderer::DepthStencil::Ptr d, 
		Renderer::Texture::Ptr dl,
		Renderer::Buffer::Ptr lightsindex = {});
	~PBR();

	void render(Renderer::Texture::Ptr rt)  override final;
private:
	void renderLightVolumes(Renderer::Texture::Ptr rt);
	void renderNormal(Renderer::Texture::Ptr rt);
private:
	Renderer::Texture::Ptr mAlbedo;
	Renderer::Texture::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;
	Renderer::Texture::Ptr mDepthLinear;
	Renderer::Buffer::Ptr mLightsIndex;

	std::array<Renderer::PixelShader::Weak,3> mPSs;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Buffer::Ptr mLightVolumesInstances[3];
	Renderer::Layout::Ptr mLightVolumeLayout;
	Mesh::Ptr mLightVolumes[3];
	Renderer::VertexShader::Weak mLightVolumeVS;
	Renderer::Buffer::Ptr mLightVolumeConstants;
	Renderer::Profile::Ptr mOnlyLighting;
};