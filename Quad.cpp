#include "Quad.h"


Quad::Quad(Renderer::Ptr r):mRenderer(r)
{
	auto vsblob = mRenderer->compileFile("quad_vs.hlsl", "main", "vs_5_0");
	mVS = mRenderer->createVertexShader((**vsblob).GetBufferPointer(), (**vsblob).GetBufferSize());
	


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

void Quad::draw(const std::array<float, 4>& color)
{
	mRenderer->clearDepth(1.0f);

	mRenderTarget.lock()->clear(color);
	mRenderer->setRenderTarget(mRenderTarget);

	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	mRenderer->setVertexBuffer(mVertexBuffer, sizeof(Vertex), 0);
	
	auto context = mRenderer->getContext();
	context->VSSetShader(*mVS.lock(), NULL, 0);
	context->PSSetShader(*mPS.lock(), NULL, 0);

	mRenderer->setLayout(mLayout.lock()->bind(mVS));
	mRenderer->setShaderResourceViews(mSRVs);
	mRenderer->setSamplers(mSamplers);
	mRenderer->setConstantBuffer(mConstant);

	context->DrawIndexed(6, 0, 0);

	mRenderer->removeRenderTargets();
	mRenderer->removeShaderResourceViews();
	mRenderer->removeSamplers();
}

void Quad::setTextures(const std::vector<Renderer::RenderTarget::Ptr>& rts)
{
	mSRVs.clear();
	for (auto i : rts)
	{
		if (i.lock())
			mSRVs.push_back((*i.lock()).getShaderResourceView());
	}
}

void Quad::setTextures(const std::vector<Renderer::Texture::Ptr>& ts)
{
	mSRVs.clear();
	for (auto i : ts)
	{
		mSRVs.push_back(*i.lock());
	}
}
