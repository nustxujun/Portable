#pragma once


#include "Pipeline.h"

class SSR :public Pipeline::Stage
{
	__declspec(align(16))
	struct Constants
	{
		Matrix view;
		Matrix proj;
		Matrix invertProj;
		float raylength;
		float width;
		float height;
		float stepstride;
	};

	struct MatrixConstants
	{
		Matrix world;
		Matrix view;
		Matrix proj;
	};
public:
	using Pipeline::Stage::Stage;
	void init();
	void render(Renderer::Texture2D::Ptr rt) override;
private:
	void renderDepthBack();
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::VertexShader::Weak mVS;
	Renderer::Texture2D::Ptr mFrame;
	Renderer::Texture2D::Ptr mDepthBack;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Buffer::Ptr mMatrixConst;
	Renderer::Sampler::Ptr mLinear;
	Renderer::Sampler::Ptr mPoint;
};