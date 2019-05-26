#include "Pipeline.h"
#include <sstream>
Pipeline::Stage::Stage(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr set, Pipeline* p):
	mRenderer(r),mScene(s), mPipeline(p)
{
	setSetting(set);
	mQuad = std::shared_ptr<Quad>(new Quad(r));
	mProfile = r->createProfile();
}

Pipeline::Stage::~Stage()
{
}

void Pipeline::Stage::update(Renderer::Texture::Ptr rt)
{
	PROFILE(mProfile);
	render(rt);
	showCost();
}


void Pipeline::Stage::showCost()
{
	std::stringstream ss;
	ss.precision(4);
	ss << mProfile.lock()->getElapsedTime();
	set(getName(), { {"cost",ss.str().c_str()}, {"type", "stage"} });
}


Pipeline::Pipeline(Renderer::Ptr r, Scene::Ptr s) :
	mRenderer(r), mScene(s)
{
	mSetting = Setting::Ptr(new Setting());
	setSetting(mSetting);
	mProfile = r->createProfile();
}

void Pipeline::pushStage(Anonymous::DrawCall dc)
{
	auto ptr = new Anonymous(mRenderer, mScene,mSetting, this, dc);
	mStages.emplace_back(Stage::Ptr(ptr));
}

void Pipeline::render()
{
	PROFILE(mProfile);
	for (auto& i : mStages)
		i->update(mFrameBuffer);

	std::stringstream ss;
	ss.precision(4);
	ss << mProfile.lock()->getElapsedTime();
	set("total", { {"cost",ss.str().c_str()}, {"type", "stage"} });
}

void Pipeline::setFrameBuffer(Renderer::Texture::Ptr rt)
{
	mFrameBuffer = rt;
}

