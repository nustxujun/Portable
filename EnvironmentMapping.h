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
		float envIntensity;
		int numprobes;
	};
public:
	enum Type
	{
		T_ONCE,
		T_EVERYFRAME,
	};


	using Pipeline::Stage::Stage;

	void init(Type type, const std::string& sky, int resolution = 512);
	void init(const std::string& cubemap);
	void render(Renderer::Texture2D::Ptr rt)  override final;
private:
	void init();
private:
	Type mType = T_ONCE;
	Renderer::Texture2D::Ptr mCube;
	std::vector<Renderer::Texture2D::Ptr> mIrradiance;
	std::vector<Renderer::Texture2D::Ptr> mPrefiltered;
	Renderer::Texture2D::Ptr mLUT;
	IrradianceCubemap::Ptr mIrradianceProcessor;
	PrefilterCubemap::Ptr mPrefilteredProcessor;
	Renderer::Buffer::Ptr mProbeInfos;
	int mNumProbes = 0;

	std::shared_ptr<Pipeline> mCubePipeline;
	std::function<void(void)> mUpdate;
	Renderer::PixelShader::Weak mPS[2];
	Renderer::Buffer::Ptr mConstants;
	Renderer::Texture2D::Ptr mFrame;
};