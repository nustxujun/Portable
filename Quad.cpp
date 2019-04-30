#include "Quad.h"


Quad::Quad(Renderer::Ptr r):mRenderer(r)
{
	auto vsblob = mRenderer->compileFile("quad_vs.hlsl", "main", "vs_5_0");
	mVS = mRenderer->createVertexShader((**vsblob).GetBufferPointer(), (**vsblob).GetBufferSize());
	

	auto psblob = mRenderer->compileFile("drawTexture_ps.hlsl", "main", "ps_5_0");
	mPS = mRenderer->createPixelShader((*psblob)->GetBufferPointer(), (*psblob)->GetBufferSize());

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

void Quad::draw(Renderer::RenderTarget::Ptr rt, std::vector<ID3D11ShaderResourceView*>& srvs, Renderer::PixelShader::Weak ps)
{
	if (ps.lock() == nullptr)
		ps = mPS;
	mRenderer->clearDepth(1.0f);
	const float colors[4] = { 0.f,0.f,0.f,1.f };
	mRenderer->clearRenderTarget(rt, colors);
	mRenderer->setRenderTarget(rt);

	auto context = mRenderer->getContext();
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	mRenderer->setVertexBuffer(mVertexBuffer, sizeof(Vertex), 0);

	context->VSSetShader(*mVS.lock(), NULL, 0);
	context->PSSetShader(*ps.lock(), NULL, 0);

	mRenderer->setLayout(mLayout.lock()->bind(mVS));
	context->PSSetShaderResources(0, srvs.size(), srvs.data());
	context->DrawIndexed(6, 0, 0);


	mRenderer->setTexture(Renderer::Texture::Ptr());
}
