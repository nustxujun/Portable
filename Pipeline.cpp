#include "Pipeline.h"

Pipeline::Stage::Stage(Renderer::Ptr r, Scene::Ptr s, Pipeline* p):
	mRenderer(r),mScene(s), mPipeline(p)
{
	mQuad = std::shared_ptr<Quad>(new Quad(r));
}

Pipeline::Stage::~Stage()
{
}


Pipeline::Pipeline(Renderer::Ptr r, Scene::Ptr s) :
	mRenderer(r), mScene(s)
{
}

void Pipeline::pushStage(Anonymous::DrawCall dc)
{
	auto ptr = new Anonymous(mRenderer, mScene, this, dc);
	mStages.emplace_back(Stage::Ptr(ptr));
}

void Pipeline::render()
{
	for (auto& i : mStages)
		i->render(mFrameBuffer);
}

void Pipeline::setFrameBuffer(Renderer::RenderTarget::Ptr rt)
{
	mFrameBuffer = rt;
}

