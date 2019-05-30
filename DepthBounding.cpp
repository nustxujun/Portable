#include "DepthBounding.h"

DepthBounding::DepthBounding(
	Renderer::Ptr r, 
	Scene::Ptr s, 
	Quad::Ptr q,
	Setting::Ptr set, 
	Pipeline * p):
	Pipeline::Stage(r,s,q,set,p), 
	mComputer(r)
{
	mName = "depth bounding";
	auto blob = r->compileFile("hlsl/depthbounding.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mConstants = r->createBuffer(sizeof(Matrix), D3D11_BIND_CONSTANT_BUFFER);
	mDepthMinMax = getUnorderedAccess("depthbounds");
	mDepth = getShaderResource("depth");

}

void DepthBounding::init(int width, int height)
{
	mWidth = (width);
	mHeight = (height);
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

	mComputer.compute(mWidth, mHeight, 1);

}
