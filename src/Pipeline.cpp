#include "Pipeline.h"
#include <sstream>
Pipeline::Stage::Stage(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q,Setting::Ptr set, Pipeline* p):
	mRenderer(r),mScene(s),mQuad(q), mPipeline(p)
{
	setSetting(set);
	mProfile = r->createProfile();
}

Pipeline::Stage::~Stage()
{
}

void Pipeline::Stage::update(Renderer::Texture2D::Ptr rt)
{
	PROFILE(mProfile);
	render(rt);
	showCost();
}


void Pipeline::Stage::showCost()
{
	set(getName(), { {"cost",mProfile.lock()->toString()}, {"type", "stage"} });
}


Pipeline::Pipeline(Renderer::Ptr r, Scene::Ptr s) :
	mRenderer(r), mScene(s)
{
	mSetting = Setting::Ptr(new Setting());
	setSetting(mSetting);
	mProfile = r->createProfile();
	mQuad = Quad::Ptr(new Quad(r));
}

void Pipeline::pushStage(const std::string& name, Anonymous::DrawCall dc)
{
	auto ptr = new Anonymous(mRenderer, mScene,mQuad,mSetting, this, name,dc);
	mStages.emplace_back(Stage::Ptr(ptr));
}

void Pipeline::render()
{
	PROFILE(mProfile);
	for (auto& i : mStages)
		i->update(mFrameBuffer);


	set("total", { {"cost",mProfile.lock()->toString()}, {"type", "stage"} });
}

void Pipeline::setFrameBuffer(Renderer::Texture2D::Ptr rt)
{
	mFrameBuffer = rt;
}


void Pipeline::addShaderResource(const std::string & name, Renderer::ShaderResource::Ptr ptr)
{
	mShaderResources[name] = ptr;
}

void Pipeline::addRenderTarget(const std::string & name, Renderer::RenderTarget::Ptr ptr)
{
	mRenderTargets[name] = ptr;
}

void Pipeline::addDepthStencil(const std::string & name, Renderer::DepthStencil::Ptr ptr)
{
	mDepthStencils[name] = ptr;
}

void Pipeline::addUnorderedAccess(const std::string & name, Renderer::UnorderedAccess::Ptr ptr)
{
	mUnorderedAccesses[name] = ptr;
}

void Pipeline::addBuffer(const std::string & name, Renderer::Buffer::Ptr ptr)
{
	mBuffers[name] = ptr;
}

void Pipeline::addTexture2D(const std::string & name, Renderer::Texture2D::Ptr ptr)
{
	mTexture2Ds[name] = ptr;
}

#define CHECK_EXISTED(C) if (C.find(name) == C.end()) report( name + " is not existed in "#C); 


Renderer::ShaderResource::Ptr Pipeline::getShaderResource(const std::string& name) 
{
	CHECK_EXISTED(mShaderResources)
	return mShaderResources[name];
}

Renderer::RenderTarget::Ptr Pipeline::getRenderTarget(const std::string& name) 
{
	CHECK_EXISTED(mRenderTargets)

	return mRenderTargets[name];
}

Renderer::DepthStencil::Ptr Pipeline::getDepthStencil(const std::string& name) 
{
	CHECK_EXISTED(mDepthStencils)

	return mDepthStencils[name];
}

Renderer::UnorderedAccess::Ptr Pipeline::getUnorderedAccess(const std::string& name) 
{
	CHECK_EXISTED(mUnorderedAccesses)

	return mUnorderedAccesses[name];
}

Renderer::Buffer::Ptr Pipeline::getBuffer(const std::string& name) 
{
	CHECK_EXISTED(mBuffers)

	return mBuffers[name];
}

Renderer::Texture2D::Ptr Pipeline::getTexture2D(const std::string & name)
{
	CHECK_EXISTED(mTexture2Ds)

	return mTexture2Ds[name];
}

void Pipeline::report(const std::string & msg)
{
	std::string str = msg + "\n";
	OutputDebugStringA("WARN --------------------\n");
	OutputDebugStringA(str.c_str());
	OutputDebugStringA("-------------------------\n");

	MessageBoxA(NULL, msg.c_str(), NULL, NULL);
	abort();

}
