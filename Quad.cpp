#include "Quad.h"


Quad::Quad(Renderer::Ptr r):mRenderer(r)
{
	auto blob = mRenderer->compileFile("quad.fx", "", "fx_5_0");
	mEffect = mRenderer->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());
	auto tech = mEffect.lock()->getTech("quad");
	
	D3DX11_PASS_DESC passDesc;
	tech->GetPassByIndex(0)->GetDesc(&passDesc);

	D3D11_INPUT_ELEMENT_DESC quadLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	mLayout = mRenderer->createLayout(quadLayout, ARRAYSIZE(quadLayout),passDesc.pIAInputSignature, passDesc.IAInputSignatureSize);


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
	mVertexBuffer = mRenderer->createBuffer(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER,&InitQuadData);

	WORD quadIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	InitQuadData.pSysMem = quadIndices;
	mIndexBuffer = mRenderer->createBuffer(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER, &InitQuadData);
}

Quad::~Quad()
{
}

void Quad::draw(ID3D11ShaderResourceView* texture)
{
	auto backbuffer = mRenderer->getBackbuffer();
	mRenderer->clearDepth(1.0f);
	const float colors[4] = { 0.f,0.f,0.f,1.f };
	mRenderer->clearRenderTarget(backbuffer, colors);
	mRenderer->setRenderTarget(backbuffer);

	auto context = mRenderer->getContext();
	context->PSSetShaderResources(0, 1, &texture);
	mRenderer->setLayout(mLayout);
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	mRenderer->setVertexBuffer(mVertexBuffer, sizeof(Vertex), 0);
	mEffect.lock()->render(context, nullptr, [](ID3D11DeviceContext* context) {
		context->DrawIndexed(6, 0, 0);
	});
}
