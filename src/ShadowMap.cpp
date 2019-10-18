#include "ShadowMap.h"
#include "MathUtilities.h"
#include <sstream>
ShadowMap::ShadowMap(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline* p) :
	Pipeline::Stage(r, s,q,st, p), mQuad(r)
{
	mName = "shadow map";


}

ShadowMap::~ShadowMap()
{
}

void ShadowMap::init(int mapsize, int numlevels, const std::vector<Renderer::Texture2D::Ptr>& rts, bool transmittance)
{
	mExponential = true;

	this->set("shadowcolor", { {"type","set"}, {"value",0.00f},{"min",0},{"max",1.0f},{"interval", "0.001"} });
	this->set("depthbias", { {"type","set"}, {"value",0.001f},{"min",0},{"max",0.01},{"interval", "0.0001"} });
	this->set("lambda", { {"type","set"}, {"value",0.5f},{"min",0},{"max",1},{"interval", "0.01"} });
	this->set("C", { {"type","set"}, {"value",80},{"min",0},{"max",100},{"interval", "1"} });

	if (transmittance)
	{
		this->set("translucency", { {"type","set"}, {"value",0.85f},{"min",0},{"max",1},{"interval", "0.01"} });
		this->set("translucency_thickness", { {"type","set"}, {"value",100},{"min",0},{"max",1000},{"interval", "1"} });
		this->set("translucency_bias", { {"type","set"}, {"value",0.003},{"min",0},{"max",0.01},{"interval", "0.0001"} });

	}

	mShadowMapSize = mapsize;
	mNumLevels = std::min(numlevels, MAX_LEVELS);
	mNumMaps = std::min((int)rts.size(), MAX_LIGHTS);
	mShadowTextures = rts;


	auto r = getRenderer();
	mGauss = ImageProcessing::create<Gaussian>(r);

	mDefaultDS = r->createDepthStencil(mShadowMapSize, mShadowMapSize, DXGI_FORMAT_D32_FLOAT);
	mDefaultCascadeDS = r->createDepthStencil(mShadowMapSize * mNumLevels, mShadowMapSize, DXGI_FORMAT_D32_FLOAT);
	mShadowMapEffect = r->createEffect("hlsl/shadowmap.fx");
	{
		auto tran = Common::format((int)transmittance);
		std::vector<std::string> flags = { "POINT", "DIR", "SPOT" };
		for (int i = 0; i < 3; ++i)
		{
			std::vector<D3D10_SHADER_MACRO> m = { {flags[i].c_str(), "1"},{"TRANSMITTANCE", tran.c_str()}, {NULL,NULL} };
			mReceiveShadowPS[i] = getRenderer()->createPixelShader("hlsl/receiveshadow.hlsl", "ps", m.data());
		}

	}
	{
		std::stringstream ss;
		ss << MAX_LIGHTS;
		D3D10_SHADER_MACRO macros[] = { {"NUM_SHADOWMAPS", ss.str().c_str()} ,{ NULL,NULL} };

		mShadowVS = r->createVertexShader("hlsl/simple_vs.hlsl", "main", macros);
	}

	//blob = r->compileFile("hlsl/castshadow.hlsl", "ps", "ps_5_0", macros);
	//mShadowPS = r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());

	//mMapParams.resize(mNumMaps);
	//for (int i = 0; i < mNumMaps; ++i)
	//{
	//	mShadowMaps.push_back(r->createDepthStencil(mShadowMapSize * mNumLevels, mShadowMapSize, DXGI_FORMAT_R32_TYPELESS, true));
	//}
	D3D11_INPUT_ELEMENT_DESC depthlayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mDepthLayout = getRenderer()->createLayout(depthlayout, ARRAYSIZE(depthlayout));




	mReceiveConstants = r->createConstantBuffer(sizeof(ReceiveConstants));


	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	mRasterizer = r->createOrGetRasterizer(rasterDesc);

	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	mShadowSampler = r->createSampler("shadow_sampler",
		D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_COMPARISON_LESS_EQUAL,
		0,0);

	mNoise = r->createTexture("media/BlueNoise.tga", 1);

}


void ShadowMap::renderToShadowMapDir(const Matrix& lightview, const Scene::Light::Cascades& cascades, Renderer::Texture2D::Ptr tex)
{
	auto effect = mShadowMapEffect.lock();
	auto renderer = getRenderer();
	if (mExponential)
	{
		effect->setTech("DirectionalLightExp");
		auto last = cascades.back();
		effect->getVariable("depthscale")->AsScalar()->SetFloat(1.0f / (last.range.y - last.range.x));
		effect->getVariable("near")->AsScalar()->SetFloat(last.range.x);
		effect->getVariable("C")->AsScalar()->SetFloat(getValue<float>("C"));

		float bg = std::exp(getValue<float>("C") );
		renderer->clearRenderTarget(tex, { bg,bg,bg,bg });
		renderer->clearDepth(mDefaultCascadeDS, 1.0f);
		renderer->setRenderTarget(tex, mDefaultCascadeDS);

	}
	else
	{
		effect->setTech("DirectionalLight");
		renderer->clearDepth(tex, 1.0f);
		renderer->setRenderTarget({}, tex);
	}

	using namespace DirectX;
	using namespace DirectX::SimpleMath;


	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->setDefaultDepthStencilState();
	renderer->setSampler(mPoint);

	renderer->setLayout(mDepthLayout.lock()->bind(mShadowVS));
	renderer->setRasterizer(mRasterizer);


	effect->getVariable("View")->AsMatrix()->SetMatrix((const float*)&lightview);
	for (int i = 0; i < mNumLevels; ++i)
	{
		D3D11_VIEWPORT vp;
		vp.Width = mShadowMapSize;
		vp.Height = mShadowMapSize;
		vp.TopLeftX = float(mShadowMapSize * i);
		vp.TopLeftY = 0;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		getRenderer()->setViewport(vp);

		effect->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)&cascades[i].proj);

		getScene()->visitRenderables([&effect, this](const Renderable& r)
		{
			effect->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
			effect->render(getRenderer(), [&effect, this,&r](ID3DX11EffectPass* pass)
			{
				getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
				getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
				getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
			});

		}, [](Scene::Entity::Ptr e) {return e->isCastShadow(); });

	}


	getRenderer()->removeRenderTargets();

	mGauss->process(tex)->get()->swap(tex);
}

void ShadowMap::renderShadowDir(const Matrix& lightview, const Scene::Light::Cascades& cascades, const Vector3& dir, Renderer::Texture2D::Ptr depth, Renderer::Texture2D::Ptr rt)
{
	ReceiveConstants constants;
	for (int j = 0; j < mNumLevels; ++j)
	{
		constants.lightProjs[j] = cascades[j].proj.Transpose();
		constants.cascadeDepths[j].x = cascades[j].cascade.y;
		constants.cascadeDepths[j].y = cascades[j].range.x;
		constants.cascadeDepths[j].z = 1.0f / (cascades[j].range.y - cascades[j].range.x);
		constants.cascadeDepths[j].w = getValue<float>("C");
	}
	constants.lightView = lightview.Transpose();

	auto cam = getCamera();
	constants.invertView = cam->getViewMatrix().Invert().Transpose();
	constants.invertProj =  cam->getProjectionMatrix().Invert().Transpose();
	constants.numcascades = mNumLevels;
	constants.scale = 1.0f / (float)mShadowMapSize;
	constants.shadowcolor = getValue<float>("shadowcolor");
	constants.depthbias = getValue<float>("depthbias");
	constants.lightdir = dir;
	constants.translucency = getValue<float>("translucency");
	constants.translucency_bias = getValue<float>("translucency_bias");
	constants.thickness = getValue<float>("translucency_thickness");
	constants.noisesize= { (float)mNoise->getDesc().Width ,(float)mNoise->getDesc().Height };
	constants.screensize = { (float)rt->getDesc().Width ,(float)rt->getDesc().Height };


	mReceiveConstants.lock()->blit(&constants, sizeof(constants));

	mQuad.setRenderTarget(rt);
	mQuad.setConstant(mReceiveConstants);
	mQuad.setSamplers({ mLinear, mPoint,mShadowSampler });
	mQuad.setDefaultBlend(false);

	mQuad.setTextures({ getShaderResource("depth"), depth , getShaderResource("normal"), mNoise });
	mQuad.setPixelShader(mReceiveShadowPS[Scene::Light::LT_DIR]);
	mQuad.setDefaultViewport();
	mQuad.draw();

}

void ShadowMap::renderToShadowMapPoint(Scene::Light::Ptr light, Renderer::Texture2D::Ptr tex)
{

	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	auto renderer = getRenderer();
	auto cam = getCamera();
	auto farZ = light->getRange();
	if (has("lightRange"))
		farZ = getValue<float>("lightRange");

	Matrix proj = DirectX::XMMatrixPerspectiveFovLH(3.14159265358f * 0.5f, 1, farZ * 0.01f, farZ);

	renderer->clearRenderTarget(tex, { farZ,0,0,0 });


	auto effect = mShadowMapEffect.lock();

	effect->setTech("PointLight");

	effect->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)&proj);
	effect->getVariable("campos")->AsVector()->SetFloatVector((const float*)& light->getNode()->getRealPosition());
	effect->getVariable("depthscale")->AsScalar()->SetFloat(1.0f / farZ);
	effect->getVariable("C")->AsScalar()->SetFloat(getValue<float>("C"));


	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->setDefaultDepthStencilState();

	renderer->setLayout(mDepthLayout.lock()->bind(mShadowVS));
	renderer->setRasterizer(mRasterizer);

	D3D11_VIEWPORT vp;
	vp.Width = mShadowMapSize;
	vp.Height = mShadowMapSize;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	renderer->setViewport(vp);


	for (int i = 0; i < 6; ++i)
	{
		renderer->clearDepth(mDefaultDS, 1.0f);
		renderer->setRenderTarget(tex->Renderer::RenderTarget::getView(i), mDefaultDS);

		auto view = light->getViewMatrix(i);
		effect->getVariable("View")->AsMatrix()->SetMatrix((const float*)&view);
		getScene()->visitRenderables([this,renderer, effect](const Renderable& r)
		{
			effect->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
			effect->render(renderer, [renderer, r](ID3DX11EffectPass* pass) {
				renderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
				renderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
				renderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
			});
		}, [](Scene::Entity::Ptr e) {return e->isCastShadow(); });

	}


	renderer->removeRenderTargets();
}

void ShadowMap::renderShadowPoint(Scene::Light::Ptr light, Renderer::Texture2D::Ptr depth, Renderer::RenderTarget::Ptr rt)
{
	ReceiveConstants constants;

	auto cam = getCamera();
	constants.invertView = cam->getViewMatrix().Invert().Transpose();
	constants.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	constants.shadowcolor = getValue<float>("shadowcolor");
	constants.depthbias = getValue<float>("depthbias");
	constants.lightdir = light->getNode()->getRealPosition();;
	constants.translucency = getValue<float>("translucency");
	constants.translucency_bias = getValue<float>("translucency_bias");
	constants.thickness = getValue<float>("translucency_thickness");

	auto farZ = light->getRange();
	if (has("lightRange"))
		farZ = getValue<float>("lightRange");
	constants.cascadeDepths[0].x = farZ * 0.01f;
	//constants.cascadeDepths[0].y = getValue<float>("C");

	constants.scale = 1.0f/ (float)mShadowMapSize;
	mReceiveConstants.lock()->blit(&constants, sizeof(constants));

	mQuad.setRenderTarget(rt);
	mQuad.setConstant(mReceiveConstants);
	mQuad.setSamplers({ mLinear, mPoint,mShadowSampler });
	mQuad.setDefaultBlend(false);

	mQuad.setTextures({ getShaderResource("depth"), depth , getShaderResource("normal") });
	mQuad.setPixelShader(mReceiveShadowPS[Scene::Light::LT_POINT]);
	mQuad.setDefaultViewport();
	mQuad.draw();
}

void ShadowMap::render(Renderer::Texture2D::Ptr rt) 
{
	std::vector<Scene::Light::Ptr> lights;
	getScene()->visitLights([&lights, this](Scene::Light::Ptr l) {
		if (l->isCastingShadow() && lights.size() < mNumMaps)
			lights.push_back(l);
	});

	for (size_t i = 0; i < lights.size(); ++i)
	{
		auto l = lights[i];
		auto type = l->getType();
		auto sm = mShadowMaps.find(l);
		if (sm == mShadowMaps.end())
		{
			switch (type)
			{
			case Scene::Light::LT_POINT:
				{
					D3D11_TEXTURE2D_DESC desc;
					desc.Width = mShadowMapSize;
					desc.Height = mShadowMapSize;
					desc.MipLevels = 1;
					desc.ArraySize = 6;
					desc.Format = DXGI_FORMAT_R32_FLOAT;
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
					desc.CPUAccessFlags = 0;
					desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
					desc.SampleDesc.Quality = 0;
					desc.SampleDesc.Count = 1;

					sm = mShadowMaps.insert({ l, getRenderer()->createTexture(desc)}).first;
					addShaderResource(Common::format(&(*l), "_shadowmap"), sm->second);
				}
				break;
			case Scene::Light::LT_DIR:
				{
					if (mExponential)
						sm = mShadowMaps.insert({ l,getRenderer()->createRenderTarget(mShadowMapSize * mNumLevels, mShadowMapSize, DXGI_FORMAT_R32G32_FLOAT) }).first;
					else
						sm = mShadowMaps.insert({ l, getRenderer()->createDepthStencil(mShadowMapSize * mNumLevels, mShadowMapSize, DXGI_FORMAT_R32_TYPELESS, true) }).first;
					addShaderResource(Common::format(&(*l), "_shadowmap"), sm->second);
				}
				break;
			case Scene::Light::LT_SPOT:
				break;
			}
			
		}

		switch (type)
		{
		case Scene::Light::LT_DIR:
			{
				l->setShadowMapParameters(mShadowMapSize,mNumLevels, getValue<float>("lambda"));
				auto cascades = l->fitToScene(getCamera());

				auto view = l->getViewMatrix();
				renderToShadowMapDir(view, cascades, sm->second);
				renderShadowDir(view, cascades, l->getDirection(), sm->second, mShadowTextures[i]);
			}
			break;
		case Scene::Light::LT_POINT:
			{
				l->setShadowMapParameters(mShadowMapSize);

				renderToShadowMapPoint(l, sm->second);
				renderShadowPoint(l, sm->second, mShadowTextures[i]);
			}
			break;
		}

	}


}
