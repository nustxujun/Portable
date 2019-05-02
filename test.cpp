#include "Test.h"
#include <DirectXMath.h>

Test::Test(Renderer::Ptr r) :mRenderer(r)
{
	auto blob = mRenderer->compileFile("test.fx", "", "fx_5_0");
	mEffect = mRenderer->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());
	mEffect.lock()->setTech("test");
	auto tech = mEffect.lock()->getTech("test");

	D3DX11_PASS_DESC passDesc;
	tech->GetPassByIndex(0)->GetDesc(&passDesc);

	D3D11_INPUT_ELEMENT_DESC quadLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	mLayout = mRenderer->createLayout(quadLayout, ARRAYSIZE(quadLayout));


	Vertex quadVertices[] =
	{
		{ 1.0f, -1.0f, 0.0f, 1.0f ,1.0f },
		{ -1.0f, -1.0f, 0.0f, 0.0f ,1.0f },
		{ -1.0f, 1.0f, 0.0f, 0.0f ,0.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f ,0.0f },
	};
	D3D11_SUBRESOURCE_DATA InitQuadData;
	ZeroMemory(&InitQuadData, sizeof(InitQuadData));
	InitQuadData.pSysMem = quadVertices;
	InitQuadData.SysMemPitch = 0;
	InitQuadData.SysMemSlicePitch = 0;
	mVertexBuffer = mRenderer->createBuffer(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER, &InitQuadData);

	WORD quadIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	InitQuadData.pSysMem = quadIndices;
	mIndexBuffer = mRenderer->createBuffer(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER, &InitQuadData);

	mRenderTarget = mRenderer->createRenderTarget(mRenderer->getWidth(), mRenderer->getHeight(), DXGI_FORMAT_R8G8B8A8_UNORM);
	mTexture = mRenderer->createTexture("Tiny_skin.dds");
	//mSampler = mRenderer->createSampler(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
}

Test::~Test()
{
}

void Test::draw(ID3D11ShaderResourceView* texture)
{
	//auto e = mEffect.lock();
	//if (e == nullptr) return;

	//auto world = e->getVariable("World")->AsMatrix();
	//auto view = e->getVariable("View")->AsMatrix();
	//auto proj = e->getVariable("Projection")->AsMatrix();
	//
	//D3DXMATRIX mat;
	//D3DXMatrixIdentity(&mat);
	//D3DXMatrixTranslation(&mat, 0, 0, 5);
	//world->SetMatrix(mat);

	//D3DXVECTOR3 eye(0,0,0);
	//D3DXVECTOR3 at(0,0,1);
	//D3DXVECTOR3 up(0,1,0);
	//D3DXMatrixLookAtLH(&mat, &eye, &at, &up);
	//view->SetMatrix(mat);

	//D3DXMatrixPerspectiveFovLH(&mat, 1.570796327f, mRenderer->getWidth() / mRenderer->getHeight(), 0.01f, 100.0f);
	//proj->SetMatrix(mat);

	//auto backbuffer = mRenderer->getBackbuffer();
	//mRenderer->clearDepth(1.0f);
	//const float colors[4] = { 0.f,0.f,0.f,1.f };
	////mRenderer->clearRenderTarget(backbuffer, colors);
	////mRenderer->setRenderTarget(backbuffer);
	//mRenderer->setRenderTarget(mRenderTarget);

	//auto context = mRenderer->getContext();

	//mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//mRenderer->setIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	//mRenderer->setVertexBuffer(mVertexBuffer, sizeof(Vertex), 0);
	//mEffect.lock()->render(mRenderer, [this, context](ID3DX11EffectPass* pass) {
	//	mRenderer->setLayout(mLayout.lock()->bind(pass));

	//	//mRenderer->setTexture(texture);
	//	//ID3D11SamplerState* ss = *(mSampler.lock());
	//	//context->PSSetSamplers(0, 1, &ss);
	//	context->DrawIndexed(6, 0, 0);
	//});
}
