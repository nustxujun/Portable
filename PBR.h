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
		float roughness;
		float metallic;
		float width;
		float height;
		int maxLightsPerTile;
		int tilePerline;
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
		Pipeline* p);
	~PBR();

	void render(Renderer::Texture::Ptr rt)  override final;
private:
	void renderLightVolumes(Renderer::Texture::Ptr rt);
	void renderNormal(Renderer::Texture::Ptr rt);
	void updateLights();
private:
	Renderer::ShaderResource::Ptr mAlbedo;
	Renderer::ShaderResource::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;
	Renderer::ShaderResource::Ptr mDepthLinear;
	Renderer::ShaderResource::Ptr mLightsIndex;

	std::array<Renderer::PixelShader::Weak,3> mPSs;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Buffer::Ptr mLightVolumesInstances[3];
	Renderer::Layout::Ptr mLightVolumeLayout;
	Mesh::Ptr mLightVolumes[3];
	Renderer::VertexShader::Weak mLightVolumeVS;
	Renderer::Buffer::Ptr mLightVolumeConstants;

	Renderer::Buffer::Ptr mLightsBuffer;
};