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

	blob = mRenderer->compileFile("drawTexture_ps.hlsl", "main", "ps_5_0");
	mDrawTexturePS = mRenderer->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

}

void DeferredRenderer::render()
{

	renderGbuffer();
	renderLightingMap();
	renderFinal();

	mQuad->setRenderTarget(mRenderer->getBackbuffer());
	mQuad->setTextures({ mFinalTarget});
	mQuad->setSamplers({ mSampleLinear});
	mQuad->setPixelShader(mDrawTexturePS);
	mQuad->draw();
}

void DeferredRenderer::renderGbuffer()
{
	using namespace DirectX;
	auto e = mSceneFX.lock();
	if (e == nullptr)
		return;

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
	mRenderer->clearRenderTargets(rts, { 0,0,0,0 });

	mRenderer->clearDepth(1.0f);
	mRenderer->setRenderTargets(rts);

	e->render(mRenderer, [world, this](ID3DX11EffectPass* pass)
	{
		mRenderer->setLayout(mGBufferLayout.lock()->bind(pass));

		mScene->visitRenderables([world, this](Renderable r)
		{
			world->SetMatrix((const float*)&r.tranformation);
			mRenderer->setTextures(r.material->mTextures);

			mRenderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			mRenderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
			mRenderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});

	});

	mRenderer->removeRenderTargets();
}

void DeferredRenderer::renderLightingMap()
{
	using namespace DirectX;
	auto cam = mScene->createOrGetCamera("main");

	LightningConstantBuffer lightningConstantBuffer;
	lightningConstantBuffer.mLightDirection = XMFLOAT4A(0.0f, -1.0f, 1.0f, 1.0f);
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
	mQuad->setRenderTarget(mFinalTarget);
	mQuad->setTextures({ mDiffuse, mLightingMap });
	mQuad->setSamplers({ mSampleLinear, mSamplePoint });
	mQuad->setPixelShader(mFinalPS);
	mQuad->draw();
}
