#pragma once

#include "Pipeline.h"
#include "GPUComputer.h"
class ClusteredLightCulling: public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertProj;
		Matrix view;
		Vector3 slicesSize;
		int numPointLights;
		int numSpotLights;
		float nearZ;
		float farZ;
	};
public:
	using Pipeline::Stage::Stage;

	void init(const Vector3& slices, const Vector3& clustersize) ;
	virtual void render(Renderer::Texture2D::Ptr rt)override;

private:
	Renderer::ComputeShader::Weak mCS;
	Renderer::Buffer::Ptr mConstants;
	GPUComputer::Ptr mComputer;
	Renderer::Buffer::Ptr mCurIndex;
	Vector3 mSlices;
	Vector3 mCluster;
	Renderer::Buffer::Ptr mFrustums;
};