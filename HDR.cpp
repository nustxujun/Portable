#include "HDR.h"

HDR::HDR(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p)
{
	mName = "HDR";
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
		mLuminance[i] = r->createRenderTarget(samplelen, samplelen, DXGI_FORMAT_R32_FLOAT);
		samplelen *= 3;
	}

	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
}

HDR::~HDR()
{
}

void HDR::render(Renderer::Texture2D::Ptr rt) 
{
	renderLuminance(rt);
	renderHDR(rt);
	auto quad = getQuad();
	quad->setRenderTarget(rt);
	quad->drawTexture(mTarget, false);

	//auto w = getRenderer()->getWidth();
	//auto h = getRenderer()->getHeight();
	//size_t count = mLuminance.size();
	//float width = (float)w / (float)count;

	//for (size_t i = 0; i < count; ++i)
	//{
	//	D3D11_VIEWPORT vp = {
	//		width * i, 0,
	//		width, width,
	//		0,1.0f,
	//	};

	//	quad->setViewport(vp);
	//	quad->setTextures({ mLuminance[i] });
	//	quad->draw();
	//}
	//
	
}

void HDR::renderLuminance(Renderer::Texture2D::Ptr rt)
{
	float len = pow(3, mLuminance.size() - 1);
	mLuminance.back().lock()->RenderTarget::clear({ 1,1,1,1 });
	auto quad = getQuad();
	quad->setDefaultBlend(false);

	quad->setRenderTarget(mLuminance.back());
	quad->setSamplers({ mPoint });
	quad->setPixelShader(mDownSamplePS2x2);
	quad->setTextures({rt });

	quad->setViewport({0,0,len ,len ,0,1.0f});
	quad->draw();

	quad->setPixelShader(mDownSamplePS3x3);
	for (size_t i = mLuminance.size() - 1; i > 0; --i)
	{
		len /= 3;
		quad->setViewport({ 0,0,len ,len ,0,1.0f });
		quad->setRenderTarget(mLuminance[i - 1]);
		quad->setTextures({ mLuminance[i] });
		quad->draw();
	}
	//quad->setRenderTarget(rt);
	//quad->drawTexture(mLuminance[0],false);
}

void HDR::renderHDR(Renderer::Texture2D::Ptr frame)
{
	auto quad = getQuad();
	quad->setDefaultViewport();
	quad->setDefaultBlend(false);

	quad->setSamplers({ mPoint });
	quad->setPixelShader(mPS);
	quad->setTextures({ frame,mLuminance[0] });
	quad->setRenderTarget(mTarget);
	quad->draw();
}
