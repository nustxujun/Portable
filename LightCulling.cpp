#include "LightCulling.h"

LightCulling::LightCulling(
	Renderer::Ptr r,
	Scene::Ptr s,
	Setting::Ptr set,
	Pipeline * p,
	Renderer::ShaderResource::Ptr depthBounds) :
	Pipeline::Stage(r, s, set, p), mComputer(r),
	mDepthBounds(depthBounds)
{
	int w = r->getWidth();
	int h = r->getHeight();
	mWidth = w;
	mHeight = h;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	mOutput = r->createTexture( desc);

	auto blob = r->compileFile("hlsl/lightculling.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
}

void LightCulling::render(Renderer::Texture::Ptr rt) 
{
	mOutput.lock()->clear({ 0,0,1.0f,0 });
	mComputer.setInputs({ mDepthBounds });
	mComputer.setOuputs({ mOutput });
	mComputer.setShader(mCS);
	mComputer.compute(mWidth, mHeight, 1);

	Quad quad(getRenderer());
	quad.setRenderTarget(rt);
	quad.drawTexture(mDepthBounds, false);
}
