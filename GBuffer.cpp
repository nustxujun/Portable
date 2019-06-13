#include "GBuffer.h"

GBuffer::GBuffer(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr st,
	Pipeline* p):
	Pipeline::Stage(r,s,q,st,p)

{
	mName = "gbuffer";
	auto blob = getRenderer()->compileFile("hlsl/gbuffer.fx", "", "fx_5_0");
	mEffect = getRenderer()->createEffect((**blob).GetBufferPointer(), (**blob).GetBufferSize());

	D3D11_INPUT_ELEMENT_DESC modelLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = getRenderer()->createLayout(modelLayout, ARRAYSIZE(modelLayout));

	mAlbedo = getRenderTarget("albedo");
	mNormal = getRenderTarget("normal");
	mDepth = getDepthStencil("depth");
}

GBuffer::~GBuffer()
{
}

void GBuffer::render(Renderer::Texture2D::Ptr rt) 
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

	std::vector<Renderer::RenderTarget::Ptr> rts = { mAlbedo, mNormal};
	renderer->clearRenderTarget(mAlbedo, { 0,0,0,0 });
	renderer->clearRenderTarget(mNormal, { 0,0,0,0 });

	renderer->clearDepthStencil(mDepth, 1.0f);
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
