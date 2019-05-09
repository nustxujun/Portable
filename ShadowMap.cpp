#include "ShadowMap.h"
#include "MathUtilities.h"

ShadowMap::ShadowMap(Renderer::Ptr r, Scene::Ptr s):Pipeline::Stage(r,s)
{
	mShadowMapSize = 1024;

	mLightCamera = getScene()->createOrGetCamera("light");
	mLightCamera->setProjectType(Scene::Camera::PT_ORTHOGRAPHIC);

	auto blob = getRenderer()->compileFile("hlsl/depth.fx", "", "fx_5_0");
	mDepthEffect = getRenderer()->createEffect((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	blob = getRenderer()->compileFile("hlsl/shadowmap.fx", "", "fx_5_0");
	mEffect = getRenderer()->createEffect((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	size_t w = getRenderer()->getWidth(); 
	size_t h = getRenderer()->getHeight();

	mLightMaps.resize(4);
	for (int i = 0; i< mLightMaps.size(); ++i)
		mLightMaps[i] = getRenderer()->createRenderTarget(mShadowMapSize, mShadowMapSize, DXGI_FORMAT_R32_FLOAT);
	mDepthStencil = r->createDepthStencil(mShadowMapSize, mShadowMapSize, DXGI_FORMAT_D32_FLOAT);

	mFinalTarget = getRenderer()->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);

	D3D11_INPUT_ELEMENT_DESC depthlayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mDepthLayout = getRenderer()->createLayout(depthlayout, ARRAYSIZE(depthlayout));

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},

	};
	mLayout = getRenderer()->createLayout(depthlayout, ARRAYSIZE(layout));

	mSampler = getRenderer()->createSampler("shadowsample", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
}

ShadowMap::~ShadowMap()
{
}

void ShadowMap::fitToScene()
{
	auto light = getScene()->createOrGetLight("main");
	auto dir = light->getDirection();

	Vector3 min = { FLT_MAX, FLT_MAX ,FLT_MAX };
	Vector3 max = { FLT_MIN, FLT_MIN, FLT_MIN };

	Vector3 up(0, 1, 0);
	if (fabs(up.Dot(dir)) >= 1.0f)
		up = { 0 ,0, 1 };
	Vector3 x = up.Cross(dir);
	x.Normalize();
	Vector3 y = dir.Cross(x);
	y.Normalize();

	auto lightspace = MathUtilities::makeMatrixFromAxis(x, y, dir);
	auto inv = lightspace.Invert();

	auto cam = getScene()->createOrGetCamera("main");
	cam->visitVisibleObject([&min,&max,inv](Scene::Entity::Ptr e)
	{
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
			Vector3 tc = Vector3::Transform(i, inv);
			min = Vector3::Min(min, tc);
			max = Vector3::Max(max, tc);
		}

	});

	Vector3 pos;
	pos.x = (min.x + max.x) * 0.5f;
	pos.y = (min.y + max.y) * 0.5f;
	pos.z = min.z ;
	pos = Vector3::Transform(pos, lightspace);

	mLightCamera->getNode()->setPosition(pos);
	mLightCamera->setDirection(dir);
	mLightCamera->setNearFar(1, max.z - min.z);
	mLightCamera->setViewport(0, 0, max.x - min.x, max.y - min.y);
}

void ShadowMap::renderToLightMap()
{
	auto e = mDepthEffect.lock();
	if (e == nullptr)
		return;

	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();


	D3D11_VIEWPORT vp;
	vp.Width = mShadowMapSize;
	vp.Height = mShadowMapSize;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.MinDepth = 0;
	vp.MaxDepth = 1.0f;
	getRenderer()->setViewport(vp);
	view->SetMatrix((const float*)&mLightCamera->getViewMatrix());
	proj->SetMatrix((const float*)&mLightCamera->getProjectionMatrix());

	for (auto& l : mLightMaps)
		l.lock()->clear({ 0,0,0,0 });
	mDepthStencil.lock()->clearDepth(1.0f);
	getRenderer()->setRenderTargets(mLightMaps, mDepthStencil);
	getRenderer()->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	getRenderer()->setDefaultBlendState();
	getRenderer()->setDefaultDepthStencilState();
	getRenderer()->setSampler(mSampler);


	getScene()->visitRenderables([world, this, e](const Renderable& r)
	{
		world->SetMatrix((const float*)&r.tranformation);
		getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

		e->render(getRenderer(), [world, this, &r](ID3DX11EffectPass* pass)
		{
			getRenderer()->setLayout(mDepthLayout.lock()->bind(pass));
			getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
	});



	getRenderer()->removeRenderTargets();
}

void ShadowMap::renderShadow()
{
	auto e = mEffect.lock();
	if (e == nullptr)
		return;

	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();
	auto lightvp = e->getVariable("LightViewProjection")->AsMatrix();
	auto cam = getScene()->createOrGetCamera("main");

	Matrix lvp = mLightCamera->getViewMatrix();
	lvp *= mLightCamera->getProjectionMatrix();
	lightvp->SetMatrix((const float*)& lvp);


	getRenderer()->setViewport(cam->getViewport());
	view->SetMatrix((const float*)&cam->getViewMatrix());
	proj->SetMatrix((const float*)&cam->getProjectionMatrix());

	mFinalTarget.lock()->clear({ 0,0,0,0 });
	getRenderer()->clearDefaultStencil(1.0f);
	getRenderer()->setRenderTarget(mFinalTarget);
	getRenderer()->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	getRenderer()->setDefaultBlendState();


	getScene()->visitRenderables([world, this, e](const Renderable& r)
	{
		world->SetMatrix((const float*)&r.tranformation);
		getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

		e->render(getRenderer(), [world, this, &r](ID3DX11EffectPass* pass)
		{
			getRenderer()->setTextures(mLightMaps);
			getRenderer()->setLayout(mDepthLayout.lock()->bind(pass));
			getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
	});



	getRenderer()->removeRenderTargets();
}

void ShadowMap::render(Renderer::RenderTarget::Ptr rt)
{
	fitToScene();
	renderToLightMap();
	renderShadow();
	
	mQuad->setRenderTarget(rt);
	mQuad->setTextures({ mFinalTarget });
	mQuad->setDefaultPixelShader();
	mQuad->setDefaultSampler();
	D3D11_BLEND_DESC desc = { 0 };
	desc.RenderTarget[0] = {
		TRUE,
		D3D11_BLEND_DEST_COLOR,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL,
	};

	mQuad->setBlend(desc);
	mQuad->draw();

	//mQuad->drawTexture(mFinalTarget);
}
