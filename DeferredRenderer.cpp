#include "DeferredRenderer.h"


DeferredRenderer::DeferredRenderer(Renderer::Ptr r, Scene::Ptr s):mRenderer(r),mScene(s)
{
	mQuad = decltype(mQuad)(new Quad(r));

	auto blob = mRenderer->compileFile("gbuffer.fx", "", "fx_5_0");
	mSceneFX = mRenderer->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());

	D3D11_INPUT_ELEMENT_DESC modelLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"BINORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mGBufferLayout = mRenderer->createLayout(modelLayout, ARRAYSIZE(modelLayout));

	int w = mRenderer->getWidth();
	int h = mRenderer->getHeight();
	mDiffuse = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mNormal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mDepth = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32_FLOAT);



	blob = r->compileFile("lighting.hlsl", "main", "ps_5_0");

	mLightingPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	mLightingMap = r->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mLightingConstants = r->createBuffer(sizeof(LightningConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);

	mSampleLinear = r->createSampler("linear", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mSamplePoint = r->createSampler("point", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	blob = mRenderer->compileFile("final.hlsl", "main", "ps_5_0");
	mFinalPS = mRenderer->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	mFinalTarget = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	mDepthStencilDesc = depthStencilDesc;

	D3D11_BLEND_DESC blend = { 0 };
	blend.RenderTarget[0] = {
		FALSE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};
	mDefaultBlendState = blend;
}

void DeferredRenderer::render()
{

	mRenderer->setDefaultBlendState();

	renderGbuffer();
	renderLightingMap();
	renderFinal();

	auto backbuffer = mRenderer->getBackbuffer();
	mQuad->setRenderTarget(backbuffer);
	mQuad->setSamplers({ mSampleLinear});
	mQuad->drawTexture(mFinalTarget);
}

void DeferredRenderer::renderGbuffer()
{
	using namespace DirectX;
	auto e = mSceneFX.lock();
	if (e == nullptr)
		return;

	//mRenderer->setDepthStencilState(mDepthStencilDesc);
	mRenderer->setDefaultDepthStencilState();

	XMMATRIX mat;
	mat = XMMatrixIdentity();
	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();

	auto cam = mScene->createOrGetCamera("main");

	mRenderer->setViewport(cam->getViewport());
	view->SetMatrix((const float*)&cam->getViewMatrix());
	proj->SetMatrix((const float*)&cam->getProjectionMatrix());

	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::vector<Renderer::RenderTarget::Ptr> rts = { mDiffuse, mNormal, mDepth };
	mDiffuse.lock()->clear({0,0,0,0});
	mNormal.lock()->clear({0,0,0,0});
	mDepth.lock()->clear({0,0,0,0});

	//mRenderer->clearRenderTargets(rts, { 1,1,1,1 });

	mRenderer->clearDefaultStencil(1.0f);
	mRenderer->setRenderTargets(rts);


	mScene->visitRenderables([world, this,e](const Renderable& r)
	{
		world->SetMatrix((const float*)&r.tranformation);
		mRenderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		mRenderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

		e->render(mRenderer, [world, this,&r](ID3DX11EffectPass* pass)
		{
			mRenderer->setTextures(r.material->mTextures);
			mRenderer->setLayout(mGBufferLayout.lock()->bind(pass));
			mRenderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
	});



	mRenderer->removeRenderTargets();
}

void DeferredRenderer::renderLightingMap()
{
	using namespace DirectX;
	auto cam = mScene->createOrGetCamera("main");
	auto light = mScene->createOrGetLight("main");
	LightningConstantBuffer lightningConstantBuffer;
	lightningConstantBuffer.mLightDirection = light->getDirection();
	lightningConstantBuffer.mLightDirection.Normalize();
	lightningConstantBuffer.mColor = XMFLOAT4A(0.99f, 1.0f, 1.0f, 1.0f);
	//XMFLOAT4 position;
	//XMStoreFloat4(&position, p_pCamera->GetPosition());
	auto campos = cam->getNode()->getPosition();
	lightningConstantBuffer.mCameraPosition = XMFLOAT3A(campos.x, campos.y, campos.z);


	//lightningConstantBuffer.mInvertViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixMultiply(p_pCamera->GetViewMatrix(), p_pCamera->GetProjectionMatrix())));
	XMMATRIX view = XMLoadFloat4x4(&cam->getViewMatrix());
	XMMATRIX proj = XMLoadFloat4x4(&cam->getProjectionMatrix());
	XMMATRIX mat = XMMatrixInverse(nullptr, view * proj);
	XMStoreFloat4x4(&lightningConstantBuffer.mInvertViewMatrix, mat);

	mRenderer->setSamplers({ mSampleLinear, mSamplePoint });

	mQuad->setBlend(mDefaultBlendState);
	mQuad->setRenderTarget(mLightingMap);
	mQuad->setTextures({ mDiffuse, mNormal, mDepth });
	mQuad->setPixelShader(mLightingPS);
	mQuad->setSamplers({ mSampleLinear, mSamplePoint });
	mQuad->setConstant(mLightingConstants);
	mLightingConstants.lock()->blit(&lightningConstantBuffer, sizeof(lightningConstantBuffer));
	mQuad->draw();

}

void DeferredRenderer::renderFinal()
{
	mQuad->setBlend(mDefaultBlendState);
	mQuad->setRenderTarget(mFinalTarget);
	mQuad->setTextures({ mDiffuse, mLightingMap });
	mQuad->setSamplers({ mSampleLinear, mSamplePoint });
	mQuad->setPixelShader(mFinalPS);
	mQuad->draw();
}
