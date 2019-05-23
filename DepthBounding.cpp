#include "DepthBounding.h"

DepthBounding::DepthBounding(
	Renderer::Ptr r, 
	Scene::Ptr s, 
	Setting::Ptr set, 
	Pipeline * p, 
	Renderer::ShaderResource::Ptr depth,
	Renderer::UnorderedAccess::Ptr db):
	Pipeline::Stage(r,s,set,p), 
	mComputer(r), 
	mDepth(depth),
	mOutput(db)
{
	auto blob = r->compileFile("hlsl/depthbounding.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	


}

void DepthBounding::render(Renderer::RenderTarget::Ptr rt)
{
	mComputer.setInputs({ mDepth });
	mComputer.setOuputs({ mOutput });
	mComputer.setShader(mCS);


	//mComputer.compute()
}
