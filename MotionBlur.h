#pragma once

#include "Pipeline.h"
class MotionBlur : public Pipeline::Stage
{
	__declspec(align(16))
		struct Constants
	{
		Matrix invertViewProj;
		Matrix lastView;
		Matrix proj;
	};
public:
	using Pipeline::Stage::Stage;

	void init();
	void render(Renderer::Texture2D::Ptr rt)  override final;

private:
	Matrix mLastViewMatrix = Matrix::Identity;
	Renderer::PixelShader::Weak mPS;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Texture2D::Ptr mFrame;
};