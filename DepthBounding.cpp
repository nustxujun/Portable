#include "DepthBounding.h"

DepthBounding::DepthBounding(
	Renderer::Ptr r, 
	Scene::Ptr s, 
	Setting::Ptr set, 
	Pipeline * p, 
	Renderer::ShaderResource::Ptr depth,
	Renderer::Texture::Ptr db):
	Pipeline::Stage(r,s,set,p), 
	mComputer(r), 
	mDepth(depth),
	mDepthMinMax(db)
{
	auto blob = r->compileFile("hlsl/depthbounding.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mConstants = r->createBuffer(sizeof(Matrix), D3D11_BIND_CONSTANT_BUFFER);
	//mDepthMinMax = r->createTexture( desc);
}

void DepthBounding::render(Renderer::Texture::Ptr rt) 
{
	auto cam = getScene()->createOrGetCamera("main");
	Matrix invertproj = cam->getProjectionMatrix().Invert().Transpose();
	mConstants.lock()->blit(&invertproj, sizeof(invertproj));

	mComputer.setConstants({ mConstants });
	mComputer.setInputs({ mDepth });
	mComputer.setOuputs({ mDepthMinMax });
	mComputer.setShader(mCS);

	auto desc = mDepthMinMax.lock()->getDesc();
	mComputer.compute(desc.Width, desc.Height, 1);

}
