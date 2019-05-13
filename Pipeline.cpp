#include "Pipeline.h"

Pipeline::Stage::Stage(Renderer::Ptr r, Scene::Ptr s, Pipeline* p):
	mRenderer(r),mScene(s), mPipeline(p)
{
	mQuad = std::shared_ptr<Quad>(new Quad(r));
}

Pipeline::Stage::~Stage()
{
}

void Pipeline::Stage::addSharedRT(const std::string & name, Renderer::RenderTarget::Ptr rt)
{
	mPipeline->mSharedRenderTargets[name] = rt;
}

void Pipeline::Stage::addSharedDS(const std::string & name, Renderer::DepthStencil::Ptr rt)
{
	mPipeline->mSharedDepthStencils[name] = rt;
}

Renderer::RenderTarget::Ptr Pipeline::Stage::getSharedRT(const std::string & name)
{
	auto ret = mPipeline->mSharedRenderTargets.find(name);
	if (ret == mPipeline->mSharedRenderTargets.end())
		return Renderer::RenderTarget::Ptr();
	else
		return ret->second;
}

Renderer::DepthStencil::Ptr Pipeline::Stage::getSharedDS(const std::string & name)
{
	auto ret = mPipeline->mSharedDepthStencils.find(name);
	if (ret == mPipeline->mSharedDepthStencils.end())
		return Renderer::DepthStencil::Ptr();
	else
		return ret->second;
	
}

Pipeline::Pipeline(Renderer::Ptr r, Scene::Ptr s) :
	mRenderer(r), mScene(s)
{
}

void Pipeline::render()
{
	auto bb = mRenderer->getBackbuffer();
	for (auto& i : mStages)
		i->render(bb);
}
