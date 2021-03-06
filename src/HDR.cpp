#include "HDR.h"

HDR::HDR(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p)
{
	mName = "HDR";

}

HDR::~HDR()
{
}

void HDR::init()
{
	this->set("brightness", { {"type","set"}, {"value",1.0f},{"min",0.1},{"max",50},{"interval", "0.1"} });
	this->set("blurcount", { {"type","set"}, {"value",5},{"min",0},{"max",5},{"interval", "1"} });
	this->set("samplescale", { {"type","set"}, {"value",1},{"min",0.1},{"max",10},{"interval", "0.1"} });


	auto r = getRenderer();
	auto blob = r->compileFile("hlsl/hdr.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());

	blob = r->compileFile("hlsl/downsample.hlsl", "downSample2x2", "ps_5_0");
	mDownSamplePS2x2 = r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());
	blob = r->compileFile("hlsl/downsample.hlsl", "downSample3x3", "ps_5_0");
	mDownSamplePS3x3 = r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());

	auto vp = getCamera()->getViewport();
	mTarget = r->createRenderTarget(vp.Width, vp.Height, DXGI_FORMAT_R32G32B32A32_FLOAT);

	const size_t numSamples = 5;
	mLuminance.resize(numSamples);
	int samplelen = 1;
	for (size_t i = 0; i < numSamples; ++i)
	{
		mLuminance[i] = r->createRenderTarget(samplelen, samplelen, DXGI_FORMAT_R32_FLOAT);
		samplelen *= 3;
	}

	mExposure = r->createRenderTarget(1, 1, DXGI_FORMAT_R32_FLOAT);
	mCalExposure = r->createPixelShader("hlsl/exposure.hlsl");

	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);


	this->set("keyvalue", { {"type","set"}, {"value",0.18f},{"min",0.01},{"max",1},{"interval", "0.01"} });
	this->set("lumMin", { {"type","set"}, {"value",0},{"min",0},{"max",2},{"interval", "0.01"} });
	this->set("lumMax", { {"type","set"}, {"value",2.0f},{"min",0},{"max",2},{"interval", "0.01"} });

	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	mBrightPass = r->createPixelShader("hlsl/brightness.hlsl", "main");

	D3D10_SHADER_MACRO macros[] = { { "HORIZONTAL", "1"}, {NULL,NULL} };
	mGaussianBlur[0] = r->createPixelShader("hlsl/gaussianblur.hlsl", "main");
	mGaussianBlur[1] = r->createPixelShader("hlsl/gaussianblur.hlsl", "main", macros);
	mBloomConstants = r->createConstantBuffer(sizeof(Vector4));

	mDownsample = ImageProcessing::create<SamplingBox>(getRenderer());
	mGaussian = ImageProcessing::create<Gaussian>(getRenderer());
}


void HDR::render(Renderer::Texture2D::Ptr rt) 
{

	renderLuminance(rt);
	renderBrightness(rt);
	renderBloom(rt);
	renderHDR(rt);



	
}

void HDR::renderLuminance(Renderer::Texture2D::Ptr rt)
{
	float len = pow(3, mLuminance.size() - 1);
	getRenderer()->clearRenderTarget(mLuminance.back(), { 1,1,1,1 });
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

	quad->setViewport({ 0,0,1,1,0,1.0f });
	quad->setRenderTarget(mExposure);
	quad->setTextures({ mLuminance[0] });
	quad->setPixelShader(mCalExposure);

	Constants c;
	c.keyvalue = getValue<float>("keyvalue");
	c.lumMin = getValue<float>("lumMin");
	c.lumMax = getValue<float>("lumMax");
	mConstants.lock()->blit(c);
	quad->setConstant(mConstants);

	quad->draw();
}

void HDR::renderHDR(Renderer::Texture2D::Ptr frame)
{
	auto quad = getQuad();

	if (mTarget.expired())
		mTarget = getRenderer()->createTexture(frame.lock()->getDesc());


	quad->setDefaultViewport();
	quad->setDefaultBlend(false);
	quad->setSamplers({ mPoint });
	quad->setPixelShader(mPS);
	quad->setTextures({ frame,mExposure });
	quad->setRenderTarget(mTarget);
	quad->draw();
	frame.lock()->swap(mTarget);
}

void HDR::renderBrightness(Renderer::Texture2D::Ptr rt)
{
	if (!mBloomRT)
	{
		mBloomRT = getRenderer()->createTemporaryRT(rt.lock()->getDesc());
	}
	auto quad = getQuad();
	quad->setTextures({ rt , mExposure});
	quad->setRenderTarget(mBloomRT->get());
	quad->setDefaultBlend(false);
	quad->setDefaultSampler();
	quad->setDefaultViewport();

	Vector4 brightness = { getValue<float>("brightness"),0,0,0 };
	mBloomConstants.lock()->blit(brightness);
	quad->setConstant(mBloomConstants);
	quad->setPixelShader(mBrightPass);
	quad->draw();
}


void HDR::renderBloom(Renderer::Texture2D::Ptr rt)
{
	if (getValue<int>("blurcount") == 0)
		return;
	mBloomRT;
	mDownsample->setScale(getValue<float>("samplescale"), getValue<float>("samplescale"));

	for (int i = 0; i < getValue<int>("blurcount"); ++i)
	{
		mBloomRT = mDownsample->process(mBloomRT->get(), 0.5);
		mBloomRT = mGaussian->process(mBloomRT->get());
	}

	for (int i = 0; i < getValue<int>("blurcount"); ++i)
	{
		mBloomRT = mDownsample->process(mBloomRT->get(), 2);
	}
	auto quad = getQuad();


	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setDefaultPixelShader();
	

	quad->setBlendColorAdd();
	quad->setRenderTarget(rt);
	quad->setTextures({ mBloomRT->get() });
	quad->draw();
}