#include "DepthBounding.h"

DepthBounding::DepthBounding(
	Renderer::Ptr r, 
	Scene::Ptr s, 
	Setting::Ptr set, 
	Pipeline * p, 
	Renderer::ShaderResource::Ptr depth):
	Pipeline::Stage(r,s,set,p), 
	mComputer(r), 
	mDepth(depth)
{
	auto blob = r->compileFile("hlsl/depthbounding.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	int w = r->getWidth();
	int h = r->getHeight();
	mWidth = ((w + 16 - 1) & ~15) / 16;
	mHeight = ((h + 16 - 1) & ~15) / 16;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = mWidth;
	desc.Height = mHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	mDepthMinMax = r->createTexture("depthbounds", desc);
}

void DepthBounding::render(Renderer::RenderTarget::Ptr rt)
{
	mComputer.setInputs({ mDepth });
	mComputer.setOuputs({ mDepthMinMax });
	mComputer.setShader(mCS);


	mComputer.compute(mWidth, mHeight, 1);

	Quad quad(getRenderer());
	quad.setRenderTarget(rt );
	quad.drawTexture(mDepthMinMax, false);
}
