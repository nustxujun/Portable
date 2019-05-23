#include "PostProcessing.h"

PostProcessing::PostProcessing(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr st, Pipeline * p, const std::string & ps):
	Pipeline::Stage(r,s,st,p), mQuad(r)
{
	auto blob = r->compileFile(ps, "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
}

PostProcessing::~PostProcessing()
{
}

void PostProcessing::render(Renderer::RenderTarget::Ptr rt)
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
