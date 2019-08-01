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
	this->set("shadowcolor", { {"type","set"}, {"value",0.00f},{"min","0"},{"max",1.0f},{"interval", "0.001"} });
	this->set("depthbias", { {"type","set"}, {"value",0.001f},{"min","0"},{"max","0.01"},{"interval", "0.0001"} });
	this->set("lambda", { {"type","set"}, {"value",0.5f},{"min","0"},{"max","1"},{"interval", "0.01"} });

	if (transmittance)
	{
		this->set("translucency", { {"type","set"}, {"value",0.85f},{"min","0"},{"max","1"},{"interval", "0.01"} });
		this->set("translucency_thickness", { {"type","set"}, {"value",100},{"min","0"},{"max","1000"},{"interval", "1"} });
		this->set("translucency_bias", { {"type","set"}, {"value",0.003},{"min","0"},{"max","0.01"},{"interval", "0.0001"} });

	}

	mShadowMapSize = mapsize;
	mNumLevels = std::min(numlevels, MAX_LEVELS);
	mNumMaps = std::min((int)rts.size(), MAX_LIGHTS);
	mShadowTextures = rts;


	auto r = getRenderer();
	
	mDefaultDS = r->createDepthStencil(mShadowMapSize, mShadowMapSize, DXGI_FORMAT_D32_FLOAT);
	
	
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
	mPointLightCast = r->createPixelShader("hlsl/pointlightdepth.hlsl");
	mPointCastPSConsts = r->createConstantBuffer(sizeof(Vector4));

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

	mCastConstants = r->createBuffer(sizeof(CastConstants), D3D11_BIND_CONSTANT_BUFFER);



	mReceiveConstants = r->createBuffer(sizeof(ReceiveConstants), D3D11_BIND_CONSTANT_BUFFER);


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

	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	mShadowSampler = r->createSampler("shadow_sampler",
		D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_TEXTURE_ADDRESS_BORDER,
		D3D11_COMPARISON_LESS_EQUAL,
		0,0);
}


void ShadowMap::renderToShadowMapDir(const Matrix& lightview, const Scene::Light::Cascades& cascades, Renderer::Texture2D::Ptr tex)
{


	getRenderer()->clearDepth(tex, 1.0f);
	//mShadowMaps[index]->getDepthStencil().lock()->clearDepth(1.0f);

	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	CastConstants constant;
	constant.view = lightview.Transpose();

	getRenderer()->setRenderTarget({}, tex);
	getRenderer()->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	getRenderer()->setDefaultBlendState();
	getRenderer()->setDefaultDepthStencilState();
	getRenderer()->setSampler(mPoint);
	getRenderer()->setVertexShader(mShadowVS);
	//getRenderer()->setPixelShader(mShadowPS);
	getRenderer()->setPixelShader({});

	getRenderer()->setLayout(mDepthLayout.lock()->bind(mShadowVS));
	getRenderer()->setRasterizer(mRasterizer);


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

		constant.proj = cascades[i].proj.Transpose();


		
		getScene()->visitRenderables([&constant, this](const Renderable& r)
		{
			constant.world = r.tranformation.Transpose();
			mCastConstants.lock()->blit(&constant, sizeof(constant));
			getRenderer()->setVSConstantBuffers({ mCastConstants });

			getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
			getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
		}, [](Scene::Entity::Ptr e) {return e->isCastShadow(); });

	}


	getRenderer()->removeRenderTargets();
}

void ShadowMap::renderShadowDir(const Matrix& lightview, const Scene::Light::Cascades& cascades, const Vector3& dir, Renderer::Texture2D::Ptr depth, Renderer::RenderTarget::Ptr rt)
{
	ReceiveConstants constants;
	for (int j = 0; j < mNumLevels; ++j)
	{
		constants.lightProjs[j] = cascades[j].proj.Transpose();
		constants.cascadeDepths[j].x = cascades[j].range.y;
	}
	constants.lightView = lightview.Transpose();

	auto cam = getCamera();
	constants.invertView = cam->getViewMatrix().Invert().Transpose();
	constants.invertProj =  cam->getProjectionMatrix().Invert().Transpose();
	constants.numcascades = mNumLevels;
	constants.scale = 1.0f / mNumLevels;
	constants.shadowcolor = getValue<float>("shadowcolor");
	constants.depthbias = getValue<float>("depthbias");
	constants.lightdir = dir;
	constants.translucency = getValue<float>("translucency");
	constants.translucency_bias = getValue<float>("translucency_bias");
	constants.thickness = getValue<float>("translucency_thickness");

	mReceiveConstants.lock()->blit(&constants, sizeof(constants));

	mQuad.setRenderTarget(rt);
	mQuad.setConstant(mReceiveConstants);
	mQuad.setSamplers({ mLinear, mPoint,mShadowSampler });
	mQuad.setDefaultBlend(false);

	mQuad.setTextures({ getShaderResource("depth"), depth , getShaderResource("normal")});
	mQuad.setPixelShader(mReceiveShadowPS[Scene::Light::LT_DIR]);
	mQuad.setDefaultViewport();
	mQuad.draw();

}

void ShadowMap::renderToShadowMapPoint(Scene::Light::Ptr light, Renderer::Texture2D::Ptr tex)
{
	auto cam = getCamera();
	Matrix proj = DirectX::XMMatrixPerspectiveFovLH(3.14159265358f * 0.5f, 1, cam->getNear(), cam->getFar());

	//mShadowMaps[index]->getDepthStencil().lock()->clearDepth(1.0f);

	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	auto renderer = getRenderer();
	CastConstants constant;

	renderer->clearRenderTarget(tex, { cam->getFar(),0,0,0 });

	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->setDefaultDepthStencilState();
	renderer->setSampler(mPoint);
	renderer->setVertexShader(mShadowVS);
	//renderer->setPixelShader(mShadowPS);
	renderer->setPixelShader(mPointLightCast);

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
	constant.proj = proj.Transpose();

	struct
	{
		Vector3 pos;
		float farZ;
	} psConsts = {
		light->getNode()->getRealPosition(),
		cam->getFar()
	};
	mPointCastPSConsts->blit(psConsts);
	renderer->setPSConstantBuffers({ mPointCastPSConsts });
	for (int i = 0; i < 6; ++i)
	{
		renderer->clearDepth(mDefaultDS, 1.0f);
		renderer->setRenderTarget(tex->Renderer::RenderTarget::getView(i), mDefaultDS);
		//renderer->getContext()->OMSetRenderTargets(0, 0, tex->Renderer::DepthStencil::getView(i));

		auto view = light->getViewMatrix(i);
		constant.view = view.Transpose();
		getScene()->visitRenderables([&constant, this,renderer](const Renderable& r)
		{
			constant.world = r.tranformation.Transpose();
			mCastConstants.lock()->blit(&constant, sizeof(constant));
			renderer->setVSConstantBuffers({ mCastConstants });

			renderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			renderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
			renderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		}, [](Scene::Entity::Ptr e) {return e->isCastShadow(); });

	}


	renderer->removeRenderTargets();
}

void ShadowMap::renderShadowPoint(const Vector3& lightpos ,Renderer::Texture2D::Ptr depth, Renderer::RenderTarget::Ptr rt)
{
	ReceiveConstants constants;

	auto cam = getCamera();
	constants.invertView = cam->getViewMatrix().Invert().Transpose();
	constants.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	constants.shadowcolor = getValue<float>("shadowcolor");
	constants.depthbias = getValue<float>("depthbias");
	constants.lightdir = lightpos;
	constants.translucency = getValue<float>("translucency");
	constants.translucency_bias = getValue<float>("translucency_bias");
	constants.thickness = getValue<float>("translucency_thickness");
	constants.cascadeDepths[0].x = cam->getFar();
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
				renderToShadowMapPoint(l, sm->second);
				renderShadowPoint(l->getNode()->getRealPosition(), sm->second, mShadowTextures[i]);
			}
			break;
		}

	}


}
