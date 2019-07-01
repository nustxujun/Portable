#pragma once

#include "Pipeline.h"
#include "ImageProcessing.h"

class EnvironmentMapping : public Pipeline::Stage
{
	__declspec(align(16))
		struct Constants
	{
		Matrix invertViewProj;
		Vector3 campos;
		float envCubeScale;
		Vector3 envCubeSize;
		float envIntensity;
		Vector3 envCubePos;
	};
public:
	using Pipeline::Stage::Stage;

	void init(const Vector3& pos, const Vector3& size,const std::string& cubemap, int resolution = 512);
	void init(const std::string& cubemap);
	void render(Renderer::Texture2D::Ptr rt)  override final;
private:
	void init();
private:
	bool mIsOnlySkybox = false;
	Renderer::Texture2D::Ptr mCube;
	Renderer::Texture2D::Ptr mIrradiance;
	Renderer::Texture2D::Ptr mPrefiltered;
	Renderer::Texture2D::Ptr mLUT;
	IrradianceCubemap::Ptr mIrradianceProcessor;
	PrefilterCubemap::Ptr mPrefilteredProcessor;


	std::shared_ptr<Pipeline> mCubePipeline;
	std::function<void(void)> mUpdate;
	Renderer::PixelShader::Weak mPS[2];
	Renderer::Buffer::Ptr mConstants;
	Renderer::Texture2D::Ptr mFrame;
	Vector3 mSize;
	Vector3 mPosition;
};