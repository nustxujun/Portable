#pragma once

#include "Pipeline.h"
#include "GPUComputer.h"
class ClusteredLightCulling: public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix invertProj;
		int numLights;
		float texelwidth;
		float texelheight;
		float nearZ;
		float farZ;
		float invclusterwidth;
		float invclusterheight;
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
};