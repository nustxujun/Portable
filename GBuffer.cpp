#include "GBuffer.h"
#include <D3DX10math.h>

GBuffer::GBuffer(Renderer::Ptr r):mRenderer(r)
{
	auto blob = mRenderer->compileFile("gbuffer.fx", "", "fx_5_0");
	mEffect = mRenderer->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());
	mEffect.lock()->setTech("gbuffer");
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

	mLayout = mRenderer->createLayout(modelLayout, ARRAYSIZE(modelLayout));

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

	D3DXVECTOR3 eye(0, 3000,  -1000);
	D3DXVECTOR3 at(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);
	D3DXMatrixLookAtLH(&mat, &eye, &at, &up);
	view->SetMatrix(mat);

	D3DXMatrixTranslation(&mat, 0, 0, 0);
	world->SetMatrix(mat);


	D3DXMatrixPerspectiveFovLH(&mat, 1.570796327f, mRenderer->getWidth() / mRenderer->getHeight(), 0.01f, 10000);
	proj->SetMatrix(mat);
	

	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mRenderer->setLayout(mLayout.lock()->bind(mEffect));
	std::vector<Renderer::RenderTarget::Ptr> rts = {mDiffuse, mNormal, mDepth};
	float colors[4] = { 0,0,0,0 };
	for (auto i : rts)
	{
		mRenderer->clearRenderTarget(i, colors);
	}
	mRenderer->clearDepth(1.0f);
	mRenderer->setRenderTargets(rts);

	e->render(mRenderer,[scene,this]()
	{
		scene->visit([this](Scene::Entity::Ptr e) 
		{
			auto mesh = e->getMesh();
			for (size_t i = 0; i < mesh->getNumMesh(); ++i)
			{
				auto submesh = mesh->getMesh(i);
				mRenderer->setTextures(submesh.material->mTextures);
				
				mRenderer->setIndexBuffer(submesh.indices, DXGI_FORMAT_R32_UINT, 0);
				mRenderer->setVertexBuffer(submesh.vertices, submesh.strideVertex, 0);
				mRenderer->getContext()->DrawIndexed(submesh.numIndices, 0, 0);
			}
		});

	});
	mRenderer->setRenderTarget(Renderer::RenderTarget::Ptr());
}

