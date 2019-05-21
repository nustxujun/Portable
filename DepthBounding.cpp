#include "DepthBounding.h"

DepthBounding::DepthBounding(Renderer::Ptr r, Scene::Ptr s, Pipeline * p):Pipeline::Stage(r,s,p), mComputer(r)
{
	auto blob = r->compileFile("hlsl/depthbounding.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	
}

void DepthBounding::render(Renderer::RenderTarget::Ptr rt)
{
	mComputer.setShader(mCS);
}
