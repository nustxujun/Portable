#include "ShadowMap.h"
#include "MathUtilities.h"

ShadowMap::ShadowMap(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q):mScene(s), mRenderer(r), mQuad(q)
{
	mShadowMapSize = 4096;
	mLightDir = { 0,-1,0.1};
	mLightDir.Normalize();

	mLightCamera = mScene->createOrGetCamera("light");
	mLightCamera->setProjectType(Scene::Camera::PT_ORTHOGRAPHIC);

	auto blob = mRenderer->compileFile("hlsl/depth.fx", "", "fx_5_0");
	mDepthEffect = mRenderer->createEffect((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	blob = mRenderer->compileFile("hlsl/shadowmap.fx", "", "fx_5_0");
	mEffect = mRenderer->createEffect((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	size_t w = mRenderer->getWidth(); 
	size_t h = mRenderer->getHeight();
	mLightMap = mRenderer->createRenderTarget(mShadowMapSize, mShadowMapSize, DXGI_FORMAT_R32_FLOAT);
	mDepthStencil = r->createDepthStencil(mShadowMapSize, mShadowMapSize, DXGI_FORMAT_D32_FLOAT);

	mFinalTarget = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);

	D3D11_INPUT_ELEMENT_DESC depthlayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mDepthLayout = mRenderer->createLayout(depthlayout, ARRAYSIZE(depthlayout));

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},

	};
	mLayout = mRenderer->createLayout(depthlayout, ARRAYSIZE(layout));

	mSampler = mRenderer->createSampler("shadowsample", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
}

ShadowMap::~ShadowMap()
{
}

void ShadowMap::fitToScene()
{
	Vector3 min = { FLT_MAX, FLT_MAX ,FLT_MAX };
	Vector3 max = { FLT_MIN, FLT_MIN, FLT_MIN };

	Vector3 up(0, 1, 0);
	if (fabs(up.Dot(mLightDir)) >= 1.0f)
		up = { 0 ,0, 1 };
	Vector3 x = up.Cross(mLightDir);
	x.Normalize();
	Vector3 y = mLightDir.Cross(x);
	y.Normalize();

	auto lightspace = MathUtilities::makeMatrixFromAxis(x, y, mLightDir);
	auto inv = lightspace.Invert();

	auto cam = mScene->createOrGetCamera("main");
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

	mLightPos.x = (min.x + max.x) * 0.5f;
	mLightPos.y = (min.y + max.y) * 0.5f;
	mLightPos.z = min.z ;
	mLightPos = Vector3::Transform(mLightPos, lightspace);

	mLightCamera->getNode()->setPosition(mLightPos);
	mLightCamera->setDirection(mLightDir);
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
	mRenderer->setViewport(vp);
	view->SetMatrix((const float*)&mLightCamera->getViewMatrix());
	proj->SetMatrix((const float*)&mLightCamera->getProjectionMatrix());

	mLightMap.lock()->clear({ 0,0,0,0 });
	mDepthStencil.lock()->clearDepth(1.0f);
	mRenderer->setRenderTarget(mLightMap, mDepthStencil);
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setDefaultBlendState();
	mRenderer->setDefaultDepthStencilState();
	mRenderer->setSampler(mSampler);


	mScene->visitRenderables([world, this, e](const Renderable& r)
	{
		world->SetMatrix((const float*)&r.tranformation);
		mRenderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		mRenderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

		e->render(mRenderer, [world, this, &r](ID3DX11EffectPass* pass)
		{
			mRenderer->setLayout(mDepthLayout.lock()->bind(pass));
			mRenderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
	});



	mRenderer->removeRenderTargets();
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
	auto cam = mScene->createOrGetCamera("main");

	Matrix lvp = mLightCamera->getViewMatrix();
	lvp *= mLightCamera->getProjectionMatrix();
	lightvp->SetMatrix((const float*)& lvp);


	mRenderer->setViewport(cam->getViewport());
	view->SetMatrix((const float*)&cam->getViewMatrix());
	proj->SetMatrix((const float*)&cam->getProjectionMatrix());

	mFinalTarget.lock()->clear({ 0,0,0,0 });
	mRenderer->clearDefaultStencil(1.0f);
	mRenderer->setRenderTarget(mFinalTarget);
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setDefaultBlendState();


	mScene->visitRenderables([world, this, e](const Renderable& r)
	{
		world->SetMatrix((const float*)&r.tranformation);
		mRenderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		mRenderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

		e->render(mRenderer, [world, this, &r](ID3DX11EffectPass* pass)
		{
			mRenderer->setTexture(mLightMap);
			mRenderer->setLayout(mDepthLayout.lock()->bind(pass));
			mRenderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
	});



	mRenderer->removeRenderTargets();
}

void ShadowMap::render()
{
	fitToScene();
	renderToLightMap();
	renderShadow();
	
	mQuad->setRenderTarget(mRenderer->getBackbuffer());
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

	//D3DX11SaveTextureToFile(mRenderer->getContext(), mFinalTarget.lock()->getTexture(), D3DX11_IFF_BMP, L"test.bmp");
	mQuad->setBlend(desc);
	mQuad->draw();

	//mQuad->drawTexture(mFinalTarget);
}
