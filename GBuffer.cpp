#include "GBuffer.h"

GBuffer::GBuffer(
	Renderer::Ptr r,
	Scene::Ptr s,
	Setting::Ptr st,
	Pipeline* p,
	Renderer::RenderTarget::Ptr a,
	Renderer::RenderTarget::Ptr n,
	Renderer::RenderTarget::Ptr w,
	Renderer::DepthStencil::Ptr d):
	Pipeline::Stage(r,s,st,p),
	mAlbedo(a), 
	mNormal(n), 
	mWorldPos(w),
	mDepth(d)
{
	auto blob = getRenderer()->compileFile("hlsl/gbuffer.fx", "", "fx_5_0");
	mEffect = getRenderer()->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());

	D3D11_INPUT_ELEMENT_DESC modelLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = getRenderer()->createLayout(modelLayout, ARRAYSIZE(modelLayout));
}

GBuffer::~GBuffer()
{
}

void GBuffer::render(Renderer::RenderTarget::Ptr rt)
{
	using namespace DirectX;
	auto e = mEffect.lock();
	if (e == nullptr)
		return;

	auto renderer = getRenderer();


	XMMATRIX mat;
	mat = XMMatrixIdentity();
	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();

	auto cam = getScene()->createOrGetCamera("main");
	view->SetMatrix((const float*)&cam->getViewMatrix());
	proj->SetMatrix((const float*)&cam->getProjectionMatrix());
	renderer->setViewport(cam->getViewport());

	std::vector<Renderer::RenderTarget::Ptr> rts = { mAlbedo, mNormal,mWorldPos};
	mAlbedo.lock()->clear({ 0,0,0,0 });
	mNormal.lock()->clear({ 0,0,0,0 });
	mWorldPos.lock()->clear({ 0,0,0 });

	mDepth.lock()->clearDepth(1.0f);
	renderer->setRenderTargets(rts, mDepth);
	renderer->setDefaultDepthStencilState();
	renderer->setDefaultRasterizer();
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	//D3D11_RASTERIZER_DESC rasterDesc;
	//rasterDesc.AntialiasedLineEnable = false;
	//rasterDesc.CullMode = D3D11_CULL_BACK;
	//rasterDesc.DepthBias = 0;
	//rasterDesc.DepthBiasClamp = 0.0f;
	//rasterDesc.DepthClipEnable = true;
	//rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	//rasterDesc.FrontCounterClockwise = false;
	//rasterDesc.MultisampleEnable = false;
	//rasterDesc.ScissorEnable = false;
	//rasterDesc.SlopeScaledDepthBias = 0.0f;
	//renderer->setRasterizer(renderer->createOrGetRasterizer(rasterDesc));

	getScene()->visitRenderables([world, this, e](const Renderable& r)
	{
		world->SetMatrix((const float*)&r.tranformation);
		getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
		if (r.material->hasTexture())
		{
			e->setTech("has_texture");
			e->render(getRenderer(), [world, this, &r](ID3DX11EffectPass* pass)
			{
				getRenderer()->setTextures(r.material->mTextures);
				getRenderer()->setLayout(mLayout.lock()->bind(pass));
				getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
			});
		}
		else
		{
			e->setTech("no_texture");
			e->render(getRenderer(), [world, this, &r](ID3DX11EffectPass* pass)
			{
				getRenderer()->setLayout(mLayout.lock()->bind(pass));
				getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
			});
		}
	});

	renderer->removeRenderTargets();
}
