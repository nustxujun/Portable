#pragma once
#include "Pipeline.h"

class PostProcessing:public Pipeline::Stage
{
public:
	PostProcessing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~PostProcessing();

	void init(const std::string& file, Renderer::Texture2D::Ptr res = {}, Renderer::Texture2D::Ptr target = {});
	void render(Renderer::Texture2D::Ptr rt)  override ;

private:
	Renderer::PixelShader::Weak mPS;
	Quad mQuad;
	Renderer::Texture2D::Ptr mTarget;
	Renderer::Texture2D::Ptr mSrc;
	Renderer::Texture2D::Ptr mMid;
};