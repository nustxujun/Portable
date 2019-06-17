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

	mAlbedo = getRenderTarget("albedo");
	mNormal = getRenderTarget("normal");
	mDepth = getDepthStencil("depth");
}

GBuffer::~GBuffer()
{
}


Renderer::Effect::Ptr GBuffer::getEffect(Material::Ptr mat)
{
	auto ret = mEffect.find(mat->getShaderID());
	if (ret != mEffect.end())
		return ret->second;

	auto macros = mat->generateShaderID();
	auto effect = getRenderer()->createEffect("hlsl/gbuffer.fx", macros.data());

	mEffect[mat->getShaderID()] = effect;
	return effect;
}


void GBuffer::render(Renderer::Texture2D::Ptr rt) 
{

	using namespace DirectX;
	auto renderer = getRenderer();

	auto cam = getScene()->createOrGetCamera("main");
	renderer->setViewport(cam->getViewport());

	std::vector<Renderer::RenderTarget::Ptr> rts = { mAlbedo, mNormal, getRenderTarget("material")};
	renderer->clearRenderTarget(mAlbedo, { 0,0,0,0 });
	renderer->clearRenderTarget(mNormal, { 0,0,0,0 });
	renderer->clearRenderTarget(getRenderTarget("material"), { 0,0,0,0 });


	renderer->clearDepthStencil(mDepth, 1.0f);
	renderer->setRenderTargets(rts, mDepth);
	renderer->setDefaultDepthStencilState();
	renderer->setDefaultRasterizer();
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();

	getScene()->visitRenderables([this,cam](const Renderable& r)
	{
		auto effect = getEffect(r.material);
		auto e = effect.lock();
		e->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
		e->getVariable("View")->AsMatrix()->SetMatrix((const float*)&cam->getViewMatrix());
		e->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)&cam->getProjectionMatrix());
		e->getVariable("roughness")->AsScalar()->SetFloat(r.material->roughness);
		e->getVariable("metallic")->AsScalar()->SetFloat(r.material->metallic);

		getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
	
		e->render(getRenderer(), [ this, &r](ID3DX11EffectPass* pass)
		{
			getRenderer()->setTextures(r.material->textures);
			getRenderer()->setLayout(r.layout.lock()->bind(pass));
			getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});

	});

	renderer->removeRenderTargets();


}
