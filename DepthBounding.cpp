#include "DepthBounding.h"

DepthBounding::DepthBounding(Renderer::Ptr r, Scene::Ptr s, Pipeline * p):Pipeline::Stage(r,s,p), mComputer(r)
{
	
}

void DepthBounding::render(Renderer::RenderTarget::Ptr rt)
{
	mComputer.setShader(mCS);
}
