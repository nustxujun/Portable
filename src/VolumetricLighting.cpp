#include "VolumetricLighting.h"

VolumetricLighting::VolumetricLighting(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p), mQuad(r)
{
	mName = "volumetric light";
}

void VolumetricLighting::init()
{
	auto r = getRenderer();
	auto blob = r->compileFile("hlsl/volumetriclight.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());


	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);
	mLinearClamp = r->createSampler("linear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	mSampleCmp = r->createSampler("shadow_sampler",
		D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_COMPARISON_LESS_EQUAL,
		0, 0);
	set("vl-numsamples", { {"type","set"}, {"value",2},{"min","0"},{"max",1024},{"interval", "1"} });
	set("vl-g", { {"type","set"}, {"value",0.5},{"min","0"},{"max",1},{"interval", "0.01"} });
	set("vl-density", { {"type","set"}, {"value",1},{"min","0"},{"max",1},{"interval", "0.01"} });

	mDownsample = ImageProcessing::create<SamplingBox>(r);
}

VolumetricLighting::~VolumetricLighting()
{
}

void VolumetricLighting::render(Renderer::Texture2D::Ptr rt) 
{
	auto tmp = mDownsample->process(rt, 0.5f);


	auto quad = getQuad();
	auto cam = getCamera();
	auto scene = getScene();
	//quad->setDefaultViewport();
	quad->setViewport({ 0,0, (float)tmp->get()->getDesc().Width, (float)tmp->get()->getDesc().Height, 0,1.0f });
	quad->setSamplers({ mLinearClamp ,mSampleCmp });
	quad->setDefaultBlend(false);
	//quad->setBlendColorAdd();
	quad->setRenderTarget(tmp->get());
	quad->setConstant(mConstants);
	quad->setPixelShader(mPS);
	Constants c;
	auto view = cam->getViewMatrix();
	c.invertViewProj = (view * cam->getProjectionMatrix()).Invert().Transpose();
	c.view = view.Transpose();
	c.campos = cam->getNode()->getRealPosition();
	c.numsamples = getValue<int>("vl-numsamples");
	c.G = getValue<float>("vl-g");
	c.maxlength = cam->getFar();
	c.density = getValue<float>("vl-density");

	scene->visitLights([quad, &c, this](Scene::Light::Ptr l)
	{
		if (l->getType() != Scene::Light::LT_DIR)
			return;

		quad->setTextures({ getShaderResource("depth") , getShaderResource(Common::format(&(*l),"_shadowmap"))});

		c.lightView = l->getViewMatrix().Transpose();
		auto cascades = l->fitToScene(getCamera());
		float* f = (float*)&c.cascadeDepths;
		for (int i = 0; i < cascades.size(); ++i)
		{
			c.lightProjs[i] = cascades[i].proj.Transpose();
			*(f++) = cascades[i].range.y;
		}
		c.numcascades = cascades.size();
		
		c.lightcolor = l->getColor();
		c.lightcolor *= getValue<float>("dirradiance");
		c.lightdir = l->getDirection();
		mConstants->blit(c);

		quad->draw();
	});


	quad->setBlendColorAdd();
	quad->setRenderTarget(rt);
	quad->setDefaultViewport();
	quad->setDefaultPixelShader();
	quad->setTextures({ tmp->get() });
	quad->draw();
}
void VolumetricLighting::renderBlur(Renderer::Texture2D::Ptr rt)
{
	auto w = getRenderer()->getWidth();
	auto h = getRenderer()->getHeight();
	auto cam = getCamera();
	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();
	Vector3 lightdir = -getScene()->createOrGetLight("main")->getDirection();
	Vector4 ld = { lightdir.x, lightdir.y, lightdir.z, 0 };

	ld = Vector4::Transform(Vector4::Transform(ld, view), proj);



	Vector4 size = { (float)w,(float)h , ld.x, ld.y};
	mConstants.lock()->blit(&size, sizeof(size));


	//mBlur->getRenderTarget().lock()->clear({ 1,1,1,1 });
	getRenderer()->clearRenderTarget(mBlur, { 1,1,1,1 });

	mQuad.setSamplers({ mLinearClamp });
	mQuad.setDefaultViewport();
	mQuad.setDefaultBlend(false);
	mQuad.setConstant(mConstants);

	mQuad.setPixelShader(mColorFilter);
	mQuad.setTextures({ rt });
	mQuad.setRenderTarget(mBlur);
	mQuad.draw();

	mQuad.setBlendColorAdd();

	mQuad.setPixelShader(mPS);
	mQuad.setTextures({ mBlur });
	mQuad.setRenderTarget(rt);

	mQuad.draw();

	getRenderer()->removeShaderResourceViews();
}
