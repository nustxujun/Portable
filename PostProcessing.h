#pragma once
#include "Pipeline.h"

class PostProcessing:public Pipeline::Stage
{
public:
	PostProcessing(Renderer::Ptr r, Scene::Ptr s, Pipeline* p, const std::string& ps);
	~PostProcessing();

	void render(Renderer::RenderTarget::Ptr rt) override ;

private:
	Renderer::PixelShader::Weak mPS;
	Quad mQuad;
	Renderer::RenderTarget::Ptr mTarget;
};