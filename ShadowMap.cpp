#include "ShadowMap.h"
#include "MathUtilities.h"

ShadowMap::ShadowMap(Renderer::Ptr r, Scene::Ptr s,Setting::Ptr st, Pipeline* p,Renderer::ShaderResource::Ptr worldpos, Renderer::ShaderResource::Ptr depth, int size, int numlevels) :
	Pipeline::Stage(r, s,st, p), mWorldPos(worldpos),mSceneDepth(depth), mShadowMapSize(size), mNumLevels(numlevels), mQuad(r)
{
	mShadowMapSize = 2048;
	mNumLevels = std::min(mNumLevels, 8);

	mProjections.resize(mNumLevels);
	mCascadeDepths.resize(mNumLevels);

	auto blob = getRenderer()->compileFile("hlsl/receiveshadow.hlsl", "ps", "ps_5_0");
	mReceiveShadowPS = getRenderer()->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	blob = r->compileFile("hlsl/castshadow.hlsl", "vs", "vs_5_0");
	mShadowVS = r->createVertexShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	size_t w = getRenderer()->getWidth();
	size_t h = getRenderer()->getHeight();

	mShadowMap = r->createDepthStencil(mShadowMapSize * mNumLevels, mShadowMapSize, DXGI_FORMAT_R32_TYPELESS, true);

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

}

ShadowMap::~ShadowMap()
{
}

void ShadowMap::fitToScene()
{

	auto light = getScene()->createOrGetLight("main");
	auto dir = light->getDirection();

	Vector3 up(0, 1, 0);
	if (fabs(up.Dot(dir)) >= 1.0f)
		up = { 0 ,0, 1 };
	Vector3 x = up.Cross(dir);
	x.Normalize();
	Vector3 y = dir.Cross(x);
	y.Normalize();

	auto lighttoworld = MathUtilities::makeMatrixFromAxis(x, y, dir);
	auto worldtolight = lighttoworld.Invert();

	auto cam = getScene()->createOrGetCamera("main");
	auto corners = cam->getWorldCorners();

	Vector3 min = { FLT_MAX, FLT_MAX ,FLT_MAX };
	Vector3 max = { FLT_MIN, FLT_MIN, FLT_MIN };

	for (auto & i : corners)
	{
		Vector3 tc = Vector3::Transform(i, worldtolight);
		min = Vector3::Min(min, tc);
		max = Vector3::Max(max, tc);
	}
	Vector3 scenemin = { FLT_MAX, FLT_MAX ,FLT_MAX };
	Vector3 scenemax = { FLT_MIN, FLT_MIN, FLT_MIN };
	cam->visitVisibleObject([&scenemin,&scenemax, worldtolight](Scene::Entity::Ptr e)
	{
		if (!e->isCastShadow()) return;

		auto aabb = e->getWorldAABB();
		const auto& omin = aabb.first;
		const auto& omax = aabb.second;
		Vector3 corners[] = {
			{omin.x , omin.y, omin.z},
			{omin.x , omin.y, omax.z},
			{omin.x , omax.y, omin.z},
			{omin.x , omax.y, omax.z},

			{omax.x , omin.y, omin.z},
			{omax.x , omin.y, omax.z},
			{omax.x , omax.y, omin.z},
			{omax.x , omax.y, omax.z},
		};

		for (const auto& i : corners)
		{
			Vector3 tc = Vector3::Transform(i, worldtolight);
			scenemin = Vector3::Min(scenemin, tc);
			scenemax = Vector3::Max(scenemax, tc);
		}
	});


	auto calClip = [this](int level, float n, float f) {
		float k = (float)level / (float)mNumLevels;

		float clog = n * std::pow(f / n, k);
		float cuni = n + (f - n) * k;

		float weight = 1;
		return clog * weight + (1 - weight) * cuni;
	};

	auto vp = cam->getViewport();
	auto fovy = cam->getFOVy();
	auto nd = cam->getNear();
	auto fd = cam->getFar();
	auto viewtoWorld = cam->getViewMatrix().Invert();
	auto camproj = cam->getProjectionMatrix();

	mLightView = worldtolight;
	for (int i = 0; i < mNumLevels; ++i)
	{
		auto n =  calClip(0, nd, fd);
		auto f = calClip(i + 1,nd,fd);

		float half = mShadowMapSize * 0.5;
		auto corners = MathUtilities::calFrustumCorners(vp.Width, vp.Height, n, f, fovy);

		Vector3 min = { FLT_MAX, FLT_MAX ,FLT_MAX };
		Vector3 max = { FLT_MIN, FLT_MIN, FLT_MIN };
		for (auto &c : corners)
		{
			c = Vector3::Transform(Vector3::Transform(c, viewtoWorld), mLightView);
			min = Vector3::Min(min, c);
			max = Vector3::Max(max, c);
		}


		mProjections[i] = DirectX::XMMatrixOrthographicOffCenterLH(min.x, max.x, min.y, max.y, scenemin.z, scenemax.z);

		Vector4 range = { 0,0,f ,1};
		range = Vector4::Transform(range, camproj);
		range /= range.w;
		mCascadeDepths[i] = range;
	}

}

void ShadowMap::renderToShadowMap()
{
	mShadowMap.lock()->clearDepth(1.0f);

	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	CastConstants constant;
	constant.view = mLightView.Transpose();

	getRenderer()->setRenderTarget({}, mShadowMap);
	getRenderer()->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	getRenderer()->setDefaultBlendState();
	getRenderer()->setDefaultDepthStencilState();
	getRenderer()->setSampler(mPoint);
	getRenderer()->setVertexShader(mShadowVS);
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

		constant.proj = mProjections[i].Transpose();


		
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

void ShadowMap::renderShadow(Renderer::RenderTarget::Ptr rt)
{
	ReceiveConstants constants;
	memcpy(constants.cascadeDepths, mCascadeDepths.data(), mCascadeDepths.size() * sizeof(Vector4));
	constants.lightView = mLightView.Transpose();
	for (int i = 0; i < mNumLevels; ++i)
	{
		constants.lightProjs[i] = mProjections[i].Transpose();
	}


	auto cam = getScene()->createOrGetCamera("main");
	constants.invertViewProj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.numcascades = mNumLevels;
	constants.scale = 1.0f / mNumLevels;
	mReceiveConstants.lock()->blit(&constants, sizeof(constants));

	mQuad.setRenderTarget(rt);
	mQuad.setConstant(mReceiveConstants);
	mQuad.setSamplers({ mLinear, mPoint });
	mQuad.setDefaultBlend();
	mQuad.setTextures({ mWorldPos, mSceneDepth,mShadowMap });
	mQuad.setPixelShader(mReceiveShadowPS);
	mQuad.setDefaultViewport();
	mQuad.draw();

}

void ShadowMap::render(Renderer::Texture::Ptr rt) 
{
	fitToScene();
	renderToShadowMap();

	renderShadow(rt);


//
//	float w = getRenderer()->getWidth();
//	float h = getRenderer()->getHeight();
//
//
//	D3D11_BLEND_DESC desc = { 0 };
//	desc.RenderTarget[0] = {
//		FALSE,
//		D3D11_BLEND_ONE,
//		D3D11_BLEND_ZERO,
//		D3D11_BLEND_OP_ADD,
//		D3D11_BLEND_ONE,
//		D3D11_BLEND_ZERO,
//		D3D11_BLEND_OP_ADD,
//		D3D11_COLOR_WRITE_ENABLE_ALL,
//	};
//
//	mQuad.setBlend(desc);
//	mQuad.setDefaultPixelShader();
//	mQuad.setDefaultSampler();
//	mQuad.setViewport({
//		0.0f,0,w, h *0.3f,0.0f, 1.0f
//	});
//	mQuad.setTextures({ mShadowMap });
//	mQuad.draw();
}
