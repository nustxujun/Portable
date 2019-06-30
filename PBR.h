#pragma once

#include "Pipeline.h"
#include "Quad.h"
class PBR:public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertViewPorj;
		Matrix view;
		Vector3 clustersize;
		int numdirs;
		Vector3 campos;
		float roughness;
		float metallic;
		float width;
		float height;
		float ambient;
		float nearZ;
		float farZ;
	};

	__declspec(align(16))
	struct LightVolumeConstants
	{
		Matrix View;
		Matrix Projection;
		int stride;
	};
public:
	PBR(
		Renderer::Ptr r,
		Scene::Ptr s,
		Quad::Ptr q,
		Setting::Ptr set,
		Pipeline* p);
	~PBR();

	void render(Renderer::Texture2D::Ptr rt)  override final;
	void init(const Vector3& cluster = { 0,0,0 }, const std::vector<Renderer::Texture2D::Ptr>& shadows = {});
private:
	void renderLightVolumes(Renderer::Texture2D::Ptr rt);
	void renderNormal(Renderer::Texture2D::Ptr rt);
private:
	Renderer::ShaderResource::Ptr mAlbedo;
	Renderer::ShaderResource::Ptr mNormal;
	Renderer::DepthStencil::Ptr mDepth;
	Renderer::Texture2D::Ptr mDepthCopy;
	Renderer::ShaderResource::Ptr mLightsIndex;
	std::vector<Renderer::Texture2D::Ptr> mShadowTextures;

	std::vector<Renderer::PixelShader::Weak> mPSs;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Buffer::Ptr mLightVolumesInstances[3];
	Renderer::Layout::Ptr mLightVolumeLayout;
	Mesh::Ptr mLightVolumes[3];
	Renderer::VertexShader::Weak mLightVolumeVS;
	Renderer::Buffer::Ptr mLightVolumeConstants;

	Vector3 mCluster;
};