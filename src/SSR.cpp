#include "SSR.h"

void SSR::init()
{

	set("raylength", { {"value", 50}, {"min", 1}, {"max", 1000}, {"interval", "1"}, {"type","set"} });
	set("stepstride", { {"value", 4}, {"min", 1}, {"max", 32}, {"interval", "1"}, {"type","set"} });
	//set("stridescale", { {"value", 1}, {"min", "0"}, {"max", "0.1"}, {"interval", "0.0001"}, {"type","set"} });
	set("reflection", { {"value", 1}, {"min", 0}, {"max", 1}, {"interval", "1"}, {"type","set"} });
	set("jitter", { {"value", 1}, {"min", 0}, {"max", 1}, {"interval", "0.001"}, {"type","set"} });
	//set("brdfBias", { {"value", 0}, {"min", "0"}, {"max", "1"}, {"interval", "0.01"}, {"type","set"} });

	setValue("ssr", true);
	mName = "ssr";
	mVS = getRenderer()->createVertexShader("hlsl/simple_vs.hlsl");
	mRayTracing = getRenderer()->createPixelShader("hlsl/ssr.hlsl", "raycast");
	mLighting = getRenderer()->createPixelShader("hlsl/ssr.hlsl", "resolve");
	mConstants = getRenderer()->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	auto vp = getCamera()->getViewport();
	mDepthBack = getRenderer()->createDepthStencil(vp.Width, vp.Height, DXGI_FORMAT_R32_TYPELESS, true);
	mMatrixConst = getRenderer()->createBuffer(sizeof(MatrixConstants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mLinear = getRenderer()->createSampler("linear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	mPoint = getRenderer()->createSampler("point_clamp", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	mSample = ImageProcessing::create<SamplingBox>(getRenderer());
	mGaussian = ImageProcessing::create<Gaussian>(getRenderer());


	mHitmap = getRenderer()->createRenderTarget(vp.Width, vp.Height, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mBlueNoise = getRenderer()->createTexture("media/BlueNoise.tga", 1);
}

void SSR::render(Renderer::Texture2D::Ptr rt)
{

	renderDepthBack();
	renderRaytrace(rt);
	renderColor(rt);


	auto quad = getQuad();
	quad->setTextures({(mFrame)});
	quad->setRenderTarget(rt);
	quad->setDefaultPixelShader();
	quad->setBlendColorAdd();


	quad->draw();
}

void SSR::renderColor(Renderer::Texture2D::Ptr rt)
{
	if (mFrame.expired())
		mFrame = getRenderer()->createTexture(rt->getDesc());
	//if (mColor.expired())
	//{
	//	auto desc = rt->getDesc();
	//	desc.MipLevels = 0;
	//	desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	//	mColor = getRenderer()->createTexture(desc);
	//}

	//auto context = getRenderer()->getContext();
	//context->CopySubresourceRegion(mColor->getTexture(), 0, 0, 0, 0, rt->getTexture(), 0, NULL);
	//context->GenerateMips(mColor->Renderer::ShaderResource::getView());
	
	auto quad = getQuad();
	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		rt,
		//mColor,
		getShaderResource("normal"),
		getShaderResource("depth"),
		{} ,
		getShaderResource("material"),
		mBlueNoise,
		mHitmap ,
	};

	if (has("envmap"))
		srvs.push_back(getShaderResource("envmap"));
	else
		srvs.push_back({});
	quad->setTextures(srvs);
	
	quad->setPixelShader(mLighting);
	quad->setRenderTarget(mFrame);
	quad->setDefaultBlend(false);
	quad->draw();

}


void SSR::renderRaytrace(Renderer::Texture2D::Ptr rt)
{
	auto cam = getCamera();
	auto vp = cam->getViewport();
	Constants c;
	c.view = cam->getViewMatrix().Transpose();
	c.proj = cam->getProjectionMatrix().Transpose();
	c.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	c.reflection = getValue<float>("reflection");
	c.raylength = getValue<float>("raylength");
	c.screenSize = { vp.Width, vp.Height };
	c.noiseSize = {(float) mBlueNoise->getDesc().Width, (float)mBlueNoise->getDesc().Height };
	c.jitter = { getValue<float>("jitter"),getValue<float>("jitter") };
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
		//mSample->process(getTexture2D("depth"),ImageProcessing::DOWN),
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

	auto cam = getCamera();
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
