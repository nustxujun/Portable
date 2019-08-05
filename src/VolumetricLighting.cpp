#include "VolumetricLighting.h"

VolumetricLighting::VolumetricLighting(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p), mQuad(r)
{
	mName = "volumetric light";
}

void VolumetricLighting::init()
{
	auto r = getRenderer();
	

	{
		std::vector<std::string> def = { "POINT","DIR" };
		for (int i = 0; i < 2; ++i)
		{
			std::vector<D3D10_SHADER_MACRO> m = { {def[i].c_str(), "1" }, {NULL,NULL} };
			mPS[i] = r->createPixelShader("hlsl/volumetriclight.hlsl", "main", m.data());
		}
	}
	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);
	mLinearClamp = r->createSampler("linear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	mSampleCmp = r->createSampler("shadow_sampler",
		D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_COMPARISON_LESS_EQUAL,
		0, 0);
	set("vl-numsamples", { {"type","set"}, {"value",32},{"min","0"},{"max",1024},{"interval", "1"} });
	set("vl-g", { {"type","set"}, {"value",0.5},{"min","0"},{"max",1},{"interval", "0.01"} });
	set("vl-density", { {"type","set"}, {"value",0.002},{"min","0"},{"max","0.1"},{"interval", "0.0001"} });

	mDownsample = ImageProcessing::create<SamplingBox>(r);
	mGauss = ImageProcessing::create<Gaussian>(r);

	D3D11_BLEND_DESC desc = {0};
	desc.RenderTarget[0] = {
	TRUE,
	D3D11_BLEND_ONE,
	D3D11_BLEND_SRC_ALPHA,
	D3D11_BLEND_OP_ADD,
	D3D11_BLEND_ONE,
	D3D11_BLEND_ONE,
	D3D11_BLEND_OP_ADD,
	D3D11_COLOR_WRITE_ENABLE_ALL
	};
	mBlend = r->createBlendState("volumetric_light", desc);

	mNoise = r->createTexture("media/BlueNoise.tga", 1);
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
	Constants c;
	auto view = cam->getViewMatrix();
	c.invertViewProj = (view * cam->getProjectionMatrix()).Invert().Transpose();
	c.view = view.Transpose();
	c.campos = cam->getNode()->getRealPosition();
	c.numsamples = getValue<int>("vl-numsamples");
	c.G = getValue<float>("vl-g");
	c.maxlength = cam->getFar();
	c.density = getValue<float>("vl-density");
	c.screenSize = {(float) rt->getDesc().Width,(float)rt->getDesc().Height };
	c.noiseSize = { (float)mNoise->getDesc().Width,(float)mNoise->getDesc().Height };

	scene->visitLights([quad, &c, this](Scene::Light::Ptr l)
	{
		auto type = l->getType();
		quad->setPixelShader(mPS[type]);
		auto shadowmap = getShaderResource(Common::format(&(*l), "_shadowmap"));
		quad->setTextures({ getShaderResource("depth") ,  shadowmap, mNoise});

		switch (type)
		{
		case Scene::Light::LT_DIR:
			{
				c.lightView = l->getViewMatrix().Transpose();
				auto cascades = l->fitToScene(getCamera()); 
					float* f = (float*)&c.cascadeDepths;
				for (int i = 0; i < cascades.size(); ++i)
				{
					c.lightProjs[i] = cascades[i].proj.Transpose();
					*(f++) = cascades[i].range.y;
				}
				c.numcascades = cascades.size();

				c.lightdir = l->getDirection();

			}
			break;
		case Scene::Light::LT_POINT:
			{
				c.numcascades = l->getShadowMapSize();
				c.lightdir = l->getNode()->getRealPosition();
				c.cascadeDepths[0].x = getCamera()->getFar();
			}
			break;
		}


		c.lightcolor = l->getColor();
		c.lightcolor *= getValue<float>("dirradiance");
		mConstants->blit(c);

		quad->draw();
	});


	auto ret = mGauss->process(tmp->get());

	quad->setBlend(mBlend);
	quad->setRenderTarget(rt);
	quad->setDefaultViewport();
	quad->setDefaultPixelShader();
	quad->setTextures({ ret->get() });
	quad->draw();
}
void VolumetricLighting::renderBlur(Renderer::Texture2D::Ptr rt)
{
	//auto w = getRenderer()->getWidth();
	//auto h = getRenderer()->getHeight();
	//auto cam = getCamera();
	//auto view = cam->getViewMatrix();
	//auto proj = cam->getProjectionMatrix();
	//Vector3 lightdir = -getScene()->createOrGetLight("main")->getDirection();
	//Vector4 ld = { lightdir.x, lightdir.y, lightdir.z, 0 };

	//ld = Vector4::Transform(Vector4::Transform(ld, view), proj);



	//Vector4 size = { (float)w,(float)h , ld.x, ld.y};
	//mConstants.lock()->blit(&size, sizeof(size));


	////mBlur->getRenderTarget().lock()->clear({ 1,1,1,1 });
	//getRenderer()->clearRenderTarget(mBlur, { 1,1,1,1 });

	//mQuad.setSamplers({ mLinearClamp });
	//mQuad.setDefaultViewport();
	//mQuad.setDefaultBlend(false);
	//mQuad.setConstant(mConstants);

	//mQuad.setPixelShader(mColorFilter);
	//mQuad.setTextures({ rt });
	//mQuad.setRenderTarget(mBlur);
	//mQuad.draw();

	//mQuad.setBlendColorAdd();

	//mQuad.setPixelShader(mPS);
	//mQuad.setTextures({ mBlur });
	//mQuad.setRenderTarget(rt);

	//mQuad.draw();

	//getRenderer()->removeShaderResourceViews();
}
