#pragma once
#include "Pipeline.h"

class PreZ :public Pipeline::Stage
{
	struct Constants
	{
		Matrix world;
		Matrix view;
		Matrix proj;
	};

public:
	using Pipeline::Stage::Stage;

	void init();
	void render(Renderer::Texture2D::Ptr rt)  override final;

private:
	Renderer::VertexShader::Weak mVS;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Layout::Ptr mLayout;
};