#include "GBuffer.h"
#include <D3DX10math.h>

GBuffer::GBuffer(Renderer::Ptr r):mRenderer(r)
{
	auto blob = mRenderer->compileFile("gbuffer.fx", "", "fx_5_0");
	mEffect = mRenderer->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());
	auto tech = mEffect.lock()->getTech("gbuffer");

	D3DX11_PASS_DESC passDesc;
	tech->GetPassByIndex(0)->GetDesc(&passDesc);

	D3D11_INPUT_ELEMENT_DESC modelLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"BINORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//{"TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = mRenderer->createLayout(modelLayout, ARRAYSIZE(modelLayout), passDesc.pIAInputSignature, passDesc.IAInputSignatureSize);

	int w = mRenderer->getWidth();
	int h = mRenderer->getHeight();
	mDiffuse = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mNormal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mDepth = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32_FLOAT);
}

GBuffer::~GBuffer()
{
}

void GBuffer::render(Scene::Ptr scene)
{
	auto e = mEffect.lock();
	if (e == nullptr)
		return;
	
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);
	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();
	world->SetMatrix(mat);
	view->SetMatrix(mat);

	D3DXMatrixPerspectiveFovLH(&mat, 1.570796327f, mRenderer->getWidth() / mRenderer->getHeight(), 0.01f, 100.0f);
	proj->SetMatrix(mat);
	

	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setLayout(mLayout);
	std::vector<Renderer::RenderTarget::Ptr> rts = {mDiffuse, mNormal, mDepth};
	float colors[4] = { 0,0,0,0 };
	for (auto i : rts)
	{
		mRenderer->clearRenderTarget(i, colors);
	}
	mRenderer->clearDepth(1.0f);
	mRenderer->setRenderTargets(rts);

	e->render(mRenderer->getContext(),nullptr,[scene,this](ID3D11DeviceContext* context)
	{
		scene->visit([context,this](Scene::Entity::Ptr e) 
		{
			auto mesh = e->getMesh();
			for (size_t i = 0; i < mesh->getNumMesh(); ++i)
			{
				auto submesh = mesh->getMesh(i);
				
				mRenderer->setIndexBuffer(submesh.indices, DXGI_FORMAT_R32_UINT, 0);
				mRenderer->setVertexBuffer(submesh.vertices, 4 * 3, 0);
				context->DrawIndexed(submesh.numIndices, 0, 0);
			}
		});

	});

}

