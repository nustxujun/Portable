#include "MotionBlur.h"

void MotionBlur::init(bool cameraonly)
{
	mName = "camera motion blur";
	if (cameraonly)
	{
		D3D10_SHADER_MACRO macro[] = { { "CAMERA_MOTION_BLUR", "1" }, {NULL,NULL} };
		mPS = getRenderer()->createPixelShader("hlsl/motionblur.hlsl","main", macro);
		mConstants = getRenderer()->createConstantBuffer(sizeof(Constants));
	}
	else
	{
		mMotionVectorEffect = getRenderer()->createEffect("hlsl/motionvector.fx");
		auto cam = getCamera();
		auto vp = cam->getViewport();
		mMotionVector = getRenderer()->createRenderTarget(vp.Width, vp.Height, DXGI_FORMAT_R32G32_FLOAT);
		addShaderResource("motionvector", mMotionVector);
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		mLayout = getRenderer()->createLayout(layout, ARRAYSIZE(layout));

		auto maxBlurPixels = (int)(5 * vp.Height / 100);
		mMaxBlurRadius = maxBlurPixels;
		auto tileSize = ((maxBlurPixels - 1) / 8 + 1) * 8;
		mTileSize = tileSize;
		auto r = getRenderer();
		mTileMaxNormlaize = ImageProcessing::create<TileMaxFilter>(r, true, mMaxBlurRadius);
		mTileMax2 = ImageProcessing::create<TileMaxFilter>(r, false, mMaxBlurRadius);

		auto tileMaxOffs = Vector2::One * (tileSize / 8.0f - 1) * -0.5f;
		mTileMaxVariable = ImageProcessing::create<TileMaxFilter>(r, tileMaxOffs, tileSize / 8);

		mNeighborMax = ImageProcessing::create<NeighborMaxFilter>(r);

		mVelocitySetup = r->createPixelShader("hlsl/motionblur.hlsl", "frag_VelocitySetup");
		mReconstruct = r->createPixelShader("hlsl/motionblur.hlsl", "test");

		mRecotConstants = getRenderer()->createConstantBuffer(sizeof(RecotConstants));
	}
}

void MotionBlur::renderCameraMB(Renderer::Texture2D::Ptr rt)
{
	if (mFrame.expired())
	{
		mFrame = getRenderer()->createTexture(rt->getDesc());
	}
	Constants constants;
	auto cam = getCamera();
	auto proj = cam->getProjectionMatrix();
	auto view = cam->getViewMatrix();
	constants.invertViewProj = (view * proj).Invert().Transpose();
	constants.lastView = mLastViewMatrix.Transpose();
	constants.proj = proj.Transpose();
	mConstants.lock()->blit(constants);

	mLastViewMatrix = view;

	auto quad = getQuad();
	quad->setDefaultBlend(false);
	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setPixelShader(mPS);
	quad->setRenderTarget(mFrame);
	quad->setConstant(mConstants);
	quad->setTextures({
		rt,
		getShaderResource("depth"),
		mMotionVector
		});
	quad->draw();

	mFrame->swap(rt);
}
void MotionBlur::render(Renderer::Texture2D::Ptr rt)
{
	if (!mPS.expired())
	{
		renderCameraMB(rt);
	}
	else
	{
		renderMotionVector();
		reconstruct(rt);
	}

}

void MotionBlur::renderMotionVector()
{
	auto depth = getDepthStencil("depth");
	auto renderer = getRenderer();
	auto scene = getScene();
	auto cam = scene->createOrGetCamera("main");

	renderer->clearRenderTarget(mMotionVector, { 0,0,0,0 });
	renderer->setRenderTarget(mMotionVector, depth);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->clearStencil(depth, 0);
	D3D11_DEPTH_STENCIL_DESC dsdesc =
	{
		TRUE,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS_EQUAL,
		FALSE,
		D3D11_DEFAULT_STENCIL_READ_MASK,
		D3D11_DEFAULT_STENCIL_WRITE_MASK,
		{
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_INCR,
			D3D11_COMPARISON_ALWAYS,
		},
		{
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_INCR,
			D3D11_COMPARISON_ALWAYS
		}
	};

	renderer->setDepthStencilState(dsdesc);
	//renderer->setSampler(mPoint);

	renderer->setDefaultRasterizer();
	renderer->setViewport(cam->getViewport());

	auto e = mMotionVectorEffect.lock();
	auto view = cam->getViewMatrix();
	e->getVariable("View")->AsMatrix()->SetMatrix((const float*)& view);
	e->getVariable("lastView")->AsMatrix()->SetMatrix((const float*)& mLastViewMatrix);
	mLastViewMatrix = view;
	e->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)& cam->getProjectionMatrix());
	e->setTech("dynamic");
	scene->visitRenderables([e, renderer, this](const Renderable& r)
	{
		if (mLastWorld.find(r.id) != mLastWorld.end())
			e->getVariable("lastWorld")->AsMatrix()->SetMatrix((const float*)&mLastWorld[r.id]);
		else
			e->getVariable("lastWorld")->AsMatrix()->SetMatrix((const float*)&Matrix::Identity);
		mLastWorld[r.id] = r.tranformation;
		e->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
		e->render(renderer, [e, renderer, this,r ](auto pass)
		{
			renderer->setLayout(mLayout.lock()->bind(pass));
			renderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			renderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
			renderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
		
	}, [](Scene::Entity::Ptr e) {
		//return !e->isStatic();
		return true;
	});

}

void MotionBlur::reconstruct(Renderer::Texture2D::Ptr rt)
{
	RecotConstants constants;
	auto cam = getCamera();
	constants.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	constants._CameraMotionVectorsTexture_TextureSize = Vector2(mMotionVector->getDesc().Width,mMotionVector->getDesc().Height);
	constants._VelocityScale = { 1,1 };
	constants._MaxBlurRadius = mMaxBlurRadius;
	constants._RcpMaxBlurRadius = 1.0f / mMaxBlurRadius;

	mRecotConstants->blit(constants);
	auto desc = rt->getDesc();
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	auto veltex = getRenderer()->createTemporaryRT(desc);
	auto quad = getQuad();
	quad->setDefaultBlend(false);
	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setPixelShader(mVelocitySetup);
	quad->setRenderTarget(veltex->get());
	quad->setConstant(mRecotConstants);
	quad->setTextures({
		rt,
		getShaderResource("depth"),
		mMotionVector
		});
	quad->draw();

	auto tile = mTileMaxNormlaize->process(veltex->get(), 0.5f); // 1/2
	tile = mTileMax2->process(tile->get(), 0.5f); // 1/4
	tile = mTileMax2->process(tile->get(), 0.5f);// 1/8
	tile = mTileMaxVariable->process(tile->get(), 1.0f / ((float)mTileSize) * 8.0f);
	
	auto neighbourMax = mNeighborMax->process(tile->get());
	

	constants._VelocityTex_TexelSize = Vector2(1.0f/(*veltex)->getDesc().Width,1.0f/ (*veltex)->getDesc().Height);
	constants._NeighborMaxTex_TexelSize =Vector2(1.0f/(*neighbourMax)->getDesc().Width,1.0f/ (*neighbourMax)->getDesc().Height);

	constants._MainTex_TexelSize = Vector2(1.0f/ rt->getDesc().Width, 1.0f/rt->getDesc().Height);

	constants._LoopCount = 32;
	mRecotConstants->blit(constants);
	quad->setPixelShader(mReconstruct);

	quad->setConstant(mRecotConstants);
	quad->setTextures({
		rt,
		{},
		mMotionVector,
		veltex->get(),
		neighbourMax->get(),
	});
	auto blur = getRenderer()->createTemporaryRT(rt->getDesc());
	quad->setRenderTarget(blur->get());
	quad->draw();
	rt->swap(blur->get());
}
