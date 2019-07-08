#pragma once

#include "Pipeline.h"

class MotionVector :public Pipeline::Stage
{
public:
	using Pipeline::Stage::Stage;

	void init();
	void render(Renderer::Texture2D::Ptr rt);

private:
	Renderer::Texture2D::Ptr mMotionVector;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
	std::unordered_map<size_t, Matrix> mLastWorld;
	Matrix mLastViewMatrix = Matrix::Identity;

};