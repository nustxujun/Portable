#pragma once

#include "Pipeline.h"
#include "ImageProcessing.h"

class EnvironmentMapping : public Pipeline::Stage
{
	__declspec(align(16))
		struct Constants
	{
		Matrix proj;
		Matrix invertViewProj;
		
		Vector3 campos;
		float nearZ;

		Vector3 probepos;
		float maxsteps;

		Vector3 intensity;
		float stepsize;

		Vector3A boxmin;
		Vector3A boxmax;
		Vector3A infmin;
		Vector3A infmax;
		
		Vector2 screenSize;
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
	Renderer::TemporaryRT::Ptr calDistance(Renderer::Texture2D::Ptr rt, const Matrix& inverViewProj);
private:
	Type mType = T_ONCE;
	std::vector<Renderer::Texture2D::Ptr> mCube;
	std::vector<Renderer::TemporaryRT::Ptr> mIrradiance;
	std::vector<Renderer::TemporaryRT::Ptr> mPrefiltered;
	std::vector<Renderer::Buffer::Ptr> mCoefficients;
	std::vector<Renderer::TemporaryRT::Ptr> mDepthCorrected;
	Renderer::Texture2D::Ptr mLUT;
	IrradianceCubemap::Ptr mIrradianceProcessor;
	PrefilterCubemap::Ptr mPrefilteredProcessor;
	//Renderer::Buffer::Ptr mProbeInfos;
	//int mNumProbes = 0;

	std::shared_ptr<Pipeline> mCubePipeline;
	std::function<void(bool)> mUpdate;
	Renderer::PixelShader::Weak mPS[3][3];
	Renderer::Buffer::Ptr mConstants;
	Renderer::Texture2D::Ptr mFrame;

	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;

	Renderer::PixelShader::Weak mCalDistance;
	Renderer::Buffer::Ptr mCalDistanceConst;
};