#pragma once

#include "Pipeline.h"

class SeparableSSS :public Pipeline::Stage
{
public:
	using Pipeline::Stage::Stage;

	void init();
	void render(Renderer::Texture2D::Ptr rt) ;

private:
	std::vector<Vector4> CalculateKernel(int nSamples,const Vector3& strength, const Vector3& falloff);
	Vector3 gaussian(float variance, float r, const Vector3& falloff);
	Vector3 profile(float r, const Vector3& falloff);


};