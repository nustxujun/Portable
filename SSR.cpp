#include "SSR.h"

void SSR::init()
{

	set("raylength", { {"value", 10}, {"min", "1"}, {"max", "1000"}, {"interval", "1"}, {"type","set"} });
	set("stepstride", { {"value", 1}, {"min", "1"}, {"max", "32"}, {"interval", "1"}, {"type","set"} });
	set("stridescale", { {"value", 1}, {"min", "0"}, {"max", "0.1"}, {"interval", "0.0001"}, {"type","set"} });
	set("reflection", { {"value", 1}, {"min", "0"}, {"max", "1"}, {"interval", "0.1"}, {"type","set"} });
	set("jitter", { {"value", 0}, {"min", "0"}, {"max", "1"}, {"interval", "0.01"}, {"type","set"} });
	set("brdfBias", { {"value", 0}, {"min", "0"}, {"max", "1"}, {"interval", "0.01"}, {"type","set"} });


	mName = "ssr";
	mVS = getRenderer()->createVertexShader("hlsl/simple_vs.hlsl");
	mRayTracing = getRenderer()->createPixelShader("hlsl/ssr.hlsl", "raymarch");
	mLighting = getRenderer()->createPixelShader("hlsl/ssr.hlsl", "filter");
	mConstants = getRenderer()->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	int w = getRenderer()->getWidth();
	int h = getRenderer()->getHeight();
	mDepthBack = getRenderer()->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
	mMatrixConst = getRenderer()->createBuffer(sizeof(MatrixConstants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mLinear = getRenderer()->createSampler("linear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	mPoint = getRenderer()->createSampler("point_clamp", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	mSample = ImageProcessing::create<SamplingBox>(getRenderer(), ImageProcessing::RT_TEMP);
	mGaussian = ImageProcessing::create<Gaussian>(getRenderer(), ImageProcessing::RT_TEMP);


	mHitmap = getRenderer()->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mBlueNoise = getRenderer()->createTexture("media/BlueNoise.tga", 1);
}

void SSR::render(Renderer::Texture2D::Ptr rt)
{

	renderDepthBack();
	renderRaytrace(rt);
	renderColor(rt);
	//auto ret = mFrame;
	//int count = 0;
	//for (int i = 0; i < count; ++i)
	//{
	//	ret = mSample->process(ret, ImageProcessing::DOWN);
	//	ret = mGaussian->process(ret);
	//}

	//for (int i = 0; i < count; ++i)
	//{
	//	ret = mSample->process(ret, ImageProcessing::UP);
	//}

	auto quad = getQuad();
	quad->setTextures({mFrame});
	quad->setRenderTarget(rt);
	quad->setDefaultPixelShader();
	//quad->setBlendColorAdd();

	D3D11_BLEND_DESC desc = { 0 };
	desc.RenderTarget[0] = {
		TRUE,
		D3D11_BLEND_SRC_ALPHA,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};

	quad->setBlend(desc);
	quad->draw();
}

void SSR::renderColor(Renderer::Texture2D::Ptr rt)
{
	if (mFrame.expired())
		mFrame = getRenderer()->createTexture(rt->getDesc());

	auto quad = getQuad();
	quad->setTextures({
		rt,
		getShaderResource("normal"),
		getShaderResource("depth"),
		{} ,
		getShaderResource("material"),
		mBlueNoise,
		mHitmap });
	
	quad->setPixelShader(mLighting);
	quad->setRenderTarget(mFrame);
	quad->setDefaultBlend(false);
	quad->draw();

}


void SSR::renderRaytrace(Renderer::Texture2D::Ptr rt)
{
	auto cam = getScene()->createOrGetCamera("main");
	Constants c;
	c.view = cam->getViewMatrix().Transpose();
	c.proj = cam->getProjectionMatrix().Transpose();
	c.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	c.reflection = getValue<float>("reflection");
	c.raylength = getValue<float>("raylength");
	c.screenSize = { (float)getRenderer()->getWidth(), (float)getRenderer()->getHeight() };
	c.noiseSize = {(float) mBlueNoise->getDesc().Width, (float)mBlueNoise->getDesc().Height };
	c.jitter = { 0,0 };
	c.stepstride = getValue<float>("stepstride");
	c.stridescale = getValue<float>("stridescale");
	c.nearZ = cam->getNear();
	c.brdfBias = getValue<float>("brdfBias");
	mConstants.lock()->blit(c);

	auto quad = getQuad();
	quad->setConstant(mConstants);
	quad->setRenderTarget(mHitmap);
	quad->setTextures({
		rt,
		getShaderResource("normal"),
		getShaderResource("depth"),
		mDepthBack ,
		getShaderResource("material"),
		mBlueNoise});
	quad->setPixelShader(mRayTracing);
	quad->setSamplers({ mLinear, mPoint });
	quad->setDefaultViewport();
	quad->setDefaultBlend(false);
	quad->draw();

}

void SSR::renderDepthBack()
{
	getRenderer()->clearDepth(mDepthBack, 1.0f);
	//mShadowMaps[index]->getDepthStencil().lock()->clearDepth(1.0f);

	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	auto cam = getScene()->createOrGetCamera("main");
	MatrixConstants constant;
	constant.view = cam->getViewMatrix().Transpose();
	constant.proj = cam->getProjectionMatrix().Transpose();

	getRenderer()->setRenderTarget({}, mDepthBack);
	getRenderer()->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	getRenderer()->setDefaultBlendState();
	getRenderer()->setDefaultDepthStencilState();
	getRenderer()->setVertexShader(mVS);
	getRenderer()->setPixelShader({});


	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_FRONT;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	getRenderer()->setRasterizer(rasterDesc);

	getRenderer()->setViewport(cam->getViewport());

	getScene()->visitRenderables([&constant, this](const Renderable& r)
	{
		constant.world = r.tranformation.Transpose();
		mMatrixConst.lock()->blit(&constant, sizeof(constant));
		getRenderer()->setVSConstantBuffers({ mMatrixConst });

		getRenderer()->setLayout(r.layout.lock()->bind(mVS));

		getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
		getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
	});


	getRenderer()->removeRenderTargets();		
}
