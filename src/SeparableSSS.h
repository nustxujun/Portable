#pragma once

#include "Pipeline.h"

class SeparableSSS :public Pipeline::Stage
{
	ALIGN16
	struct Constants
	{
		Matrix invertProj;
		Vector2 texelsize;
		float distToProjWin;
		float width;
	};
public:
	using Pipeline::Stage::Stage;

	void init(int numSamples = 25);
	void render(Renderer::Texture2D::Ptr rt) ;

private:
	std::vector<Vector4> CalculateKernel(int nSamples,const Vector3& strength, const Vector3& falloff);
	Vector3 gaussian(float variance, float r, const Vector3& falloff);
	Vector3 profile(float r, const Vector3& falloff);

private:
	Renderer::PixelShader::Weak mPS[2];
	Renderer::Buffer::Ptr mKernelConst;
	Renderer::Buffer::Ptr mConstants ;
	Renderer::Sampler::Ptr mPoint;
	Renderer::Sampler::Ptr mLinear;
};