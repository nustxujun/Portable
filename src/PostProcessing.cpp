#include "PostProcessing.h"

PostProcessing::PostProcessing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p), mQuad(r)
{
}

void PostProcessing::init(const std::string& file, Renderer::Texture2D::Ptr res, Renderer::Texture2D::Ptr target)
{
	mName = file;
	mPS = getRenderer()->createPixelShader(file, "main");
	mSrc = res;
	mTarget = target;

}


PostProcessing::~PostProcessing()
{
}

void PostProcessing::render(Renderer::Texture2D::Ptr rt) 
{
	if (mTarget.lock() == nullptr)
		mTarget = rt;
	if (mSrc.lock() == nullptr)
		mSrc = rt;

	if (mMid.lock() == nullptr)
		mMid = getRenderer()->createTexture(mTarget.lock()->getDesc());


	mQuad.setDefaultSampler();
	mQuad.setDefaultViewport();
	mQuad.setDefaultBlend(false);
	mQuad.setPixelShader(mPS);
	mQuad.setTextures({ mSrc });
	mQuad.setRenderTarget(mMid);
	mQuad.draw();


	mTarget.lock()->swap(mMid);

	//if (sametex)
	//{
	//	mQuad.setRenderTarget(mTarget);
	//	mQuad.drawTexture(mMid, false);
	//}
}
