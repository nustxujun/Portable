#pragma once

#include "Pipeline.h"

class TAA :public Pipeline::Stage
{

	ALIGN16 struct Constants
	{
		Vector2 jitter;
		Vector2 texsize;
	};
public:
	using Pipeline::Stage::Stage;

	void init();
	void render(Renderer::Texture2D::Ptr rt);
private:
	void offsetCamera();
	void blend(Renderer::Texture2D::Ptr rt);
private:
	Renderer::TemporaryRT::Ptr mHistory;
	Renderer::PixelShader::Weak mBlend;
	Renderer::Buffer::Ptr mConstants;
	Vector2 mJitter = Vector2::Zero;
};