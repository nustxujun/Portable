#include "Quad.h"


Quad::Quad(Renderer::Ptr r):mRenderer(r)
{
	mViewport = 
	{
		0,0,
		(float)r->getWidth(), (float)r->getHeight(),
		0, 1.0f
	};


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

	auto blob = mRenderer->compileFile("drawTexture_ps.hlsl", "main", "ps_5_0");
	mDrawTexturePS = mRenderer->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mDefaultSampler = r->createSampler("point", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

}

Quad::~Quad()
{
}

void Quad::draw(const std::array<float, 4>& color)
{
	mRenderer->clearDefaultStencil(1.0f);

	//mRenderTarget.lock()->clear(color);
	mRenderer->setRenderTarget(mRenderTarget);

	mRenderer->setViewport(mViewport);

	mRenderer->setBlendState(mBlendState.desc, mBlendState.factor, mBlendState.mask);
	mRenderer->setDefaultDepthStencilState();
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	mRenderer->setVertexBuffer(mVertexBuffer, sizeof(Vertex), 0);
	


	//context->VSSetShader(*mVS.lock(), NULL, 0);
	//context->PSSetShader(*mPS.lock(), NULL, 0);
	mRenderer->setVertexShader(mVS);
	mRenderer->setPixelShader(mPS);

	mRenderer->setLayout(mLayout.lock()->bind(mVS));
	mRenderer->setTextures(mSRVs);
	mRenderer->setSamplers(mSamplers);
	mRenderer->setPSConstantBuffers(mConstants);

	auto context = mRenderer->getContext();
	context->DrawIndexed(6, 0, 0);

	mRenderer->removeRenderTargets();
	mRenderer->removeShaderResourceViews();
	mRenderer->removeSamplers();
}



void Quad::setBlend(const D3D11_BLEND_DESC & desc, const std::array<float, 4>& factor, size_t mask)
{
	mBlendState = { desc,factor, mask };
}

void Quad::setDefaultBlend(bool blend)
{
	D3D11_BLEND_DESC desc = { 0 };

	desc.RenderTarget[0] = {
		blend,
		D3D11_BLEND_DEST_COLOR,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};
	mBlendState = {
		desc,
		{1,1,1,1},
		0xffffffff,
	};
}
