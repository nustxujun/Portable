#pragma once
#include "Pipeline.h"

class PostProcessing:public Pipeline::Stage
{
public:
	PostProcessing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~PostProcessing();

	void init(const std::string& file);
	void render(Renderer::Texture::Ptr rt)  override ;

private:
	Renderer::PixelShader::Weak mPS;
	Quad mQuad;
	Renderer::Texture::Ptr mTarget;
};