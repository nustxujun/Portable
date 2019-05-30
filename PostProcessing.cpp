#include "PostProcessing.h"

PostProcessing::PostProcessing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p), mQuad(r)
{
}

void PostProcessing::init(const std::string& file)
{
	mName = file;
	mPS = getRenderer()->createPixelShader(file, "main");
}


PostProcessing::~PostProcessing()
{
}

void PostProcessing::render(Renderer::Texture::Ptr rt) 
{
	if (mTarget.lock() == nullptr)
		mTarget = rt.lock()->clone();

	mQuad.setDefaultSampler();
	mQuad.setDefaultViewport();
	mQuad.setDefaultBlend(false);
	mQuad.setPixelShader(mPS);
	mQuad.setTextures({ rt });
	mQuad.setRenderTarget(mTarget);
	mQuad.draw();

	rt.lock()->swap(mTarget);


	//mQuad.setRenderTarget(rt);
	//mQuad.drawTexture(mTarget, false);
}
