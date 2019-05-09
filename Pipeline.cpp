#include "Pipeline.h"

Pipeline::Stage::Stage(Renderer::Ptr r, Scene::Ptr s):
	mRenderer(r),mScene(s)
{
	mQuad = std::shared_ptr<Quad>(new Quad(r));
}

Pipeline::Stage::~Stage()
{
}

Pipeline::Pipeline(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q) :
	mRenderer(r), mScene(s)
{
}

void Pipeline::render()
{
	auto bb = mRenderer->getBackbuffer();
	for (auto& i : mStages)
		i->render(bb);
}
