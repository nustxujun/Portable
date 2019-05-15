#include "HDR.h"

HDR::HDR(Renderer::Ptr r, Scene::Ptr s, Pipeline * p):
	Pipeline::Stage(r,s,p), mQuad(r)
{
	auto blob = r->compileFile("hlsl/hdr.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	blob = r->compileFile("hlsl/downsample.hlsl", "downSample2x2", "ps_5_0");
	mDownSamplePS2x2 = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	blob = r->compileFile("hlsl/downsample.hlsl", "downSample3x3", "ps_5_0");
	mDownSamplePS3x3 = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	auto w = r->getWidth();
	auto h = r->getHeight();
	mTarget = r->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);

	const size_t numSamples = 5;
	mLuminance.resize(numSamples);
	int samplelen = 1;
	for (size_t i = 0; i < numSamples; ++i)
	{
		mLuminance[i] = r->createRenderTarget(samplelen, samplelen, DXGI_FORMAT_R32G32B32A32_FLOAT);
		samplelen *= 3;
	}

	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
}

HDR::~HDR()
{
}

void HDR::render(Renderer::RenderTarget::Ptr rt)
{
	renderLuminance(rt);

	mQuad.setDefaultViewport();
	mQuad.setDefaultBlend(false);

	mQuad.setSamplers({ mPoint });
	mQuad.setPixelShader(mPS);
	mQuad.setTextures({ rt,mLuminance[0] });
	mQuad.setRenderTarget(mTarget);
	mQuad.draw();

	mQuad.setRenderTarget(rt);
	mQuad.drawTexture(mTarget,false);
}

void HDR::renderLuminance(Renderer::RenderTarget::Ptr rt)
{
	mLuminance.back().lock()->clear({ 1,1,1,1 });
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultViewport();

	mQuad.setRenderTarget(mLuminance.back());
	mQuad.setSamplers({ mPoint });
	mQuad.setPixelShader(mDownSamplePS3x3);
	mQuad.setTextures({rt });
	mQuad.draw();

	mQuad.setPixelShader(mDownSamplePS2x2);
	for (size_t i = mLuminance.size() - 1; i > 0; --i)
	{
		mQuad.setRenderTarget(mLuminance[i - 1]);
		mQuad.setTextures({ mLuminance[i] });
		mQuad.draw();
	}
	//mQuad.setRenderTarget(rt);
	//mQuad.drawTexture(mLuminance[0],false);
}
