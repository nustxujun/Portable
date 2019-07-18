#include "GBuffer.h"

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

	auto cam = getCamera();
	renderer->setViewport(cam->getViewport());

	std::vector<Renderer::RenderTarget::Ptr> rts = { mAlbedo, mNormal, getRenderTarget("material")};
	//renderer->clearRenderTarget(mAlbedo, { 0,0,0,0 });
	//renderer->clearRenderTarget(mNormal, { 0,0,0,0 });
	//renderer->clearRenderTarget(getRenderTarget("material"), { 0,0,0,0 });
	if (mClearDepth)
		renderer->clearDepth(mDepth,1.0f);

	//renderer->clearDepthStencil(mDepth, 1.0f);
	renderer->setRenderTargets(rts, mDepth);
	//renderer->setDefaultRasterizer();
	renderer->setRasterizer(mRasterizer);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();



	renderer->setDepthStencilState("depth_write_less_equal");


	getScene()->visitRenderables([this,cam](const Renderable& r)
	{
		auto effect = getEffect(r.material);
		auto e = effect.lock();
		e->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
		e->getVariable("View")->AsMatrix()->SetMatrix((const float*)&cam->getViewMatrix());
		e->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)&cam->getProjectionMatrix());
		e->getVariable("roughness")->AsScalar()->SetFloat(r.material->roughness * getValue<float>("roughness"));
		e->getVariable("metallic")->AsScalar()->SetFloat(r.material->metallic * getValue<float>("metallic"));
		e->getVariable("reflection")->AsScalar()->SetFloat(r.material->reflection);
		e->getVariable("diffuse")->AsVector()->SetFloatVector((const float*)&r.material->diffuse);

		e->getVariable("campos")->AsVector()->SetFloatVector((const float*)&cam->getNode()->getRealPosition());
		e->getVariable("heightscale")->AsScalar()->SetFloat(getValue<float>("heightscale"));
		e->getVariable("minsamplecount")->AsScalar()->SetInt(getValue<int>("minSampleCount"));
		e->getVariable("maxsamplecount")->AsScalar()->SetInt(getValue<int>("maxSampleCount"));

		auto& texs = r.material->textures;
		std::vector<ID3D11ShaderResourceView*> srvs;
		for (auto&i : texs)
			srvs.push_back(i->Renderer::ShaderResource::getView());
		if (!texs.empty())
			e->getVariable("diffuseTex")->AsShaderResource()->SetResourceArray(srvs.data(), 0, srvs.size());

		getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
	
		e->render(getRenderer(), [ this, &r](ID3DX11EffectPass* pass)
		{
			getRenderer()->setTextures(r.material->textures);
			getRenderer()->setLayout(r.layout.lock()->bind(pass));
			getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});

	}, mRenderCond);

	renderer->removeRenderTargets();


}

void GBuffer::init(bool cleardepth, std::function<bool(Scene::Entity::Ptr)> cond)
{
	mName = "gbuffer";
	mClearDepth = cleardepth;
	set("heightscale", { {"type","set"}, {"value",0.05f},{"min","0"},{"max",2},{"interval", "0.01"} });
	set("minSampleCount", { {"type","set"}, {"value",8},{"min","1"},{"max",1000},{"interval", "1"} });
	set("maxSampleCount", { {"type","set"}, {"value",100},{"min","1"},{"max",1000},{"interval", "1"} });

	auto cam = getCamera();
	auto vp = cam->getViewport();
	auto w = vp.Width;
	auto h = vp.Height;
	auto renderer = getRenderer();
	auto albedo = renderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	addShaderResource("albedo", albedo);
	addRenderTarget("albedo", albedo);
	auto normal = renderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	addShaderResource("normal", normal);
	addRenderTarget("normal", normal);
	auto depth = renderer->createDepthStencil(w, h, DXGI_FORMAT_R24G8_TYPELESS, true);
	addShaderResource("depth", depth);
	addDepthStencil("depth", depth);
	addTexture2D("depth", depth);
	auto material = renderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	addShaderResource("material", material);
	addRenderTarget("material", material);


	mAlbedo = getRenderTarget("albedo");
	mNormal = getRenderTarget("normal");
	mDepth = getDepthStencil("depth");


	mRenderCond = cond;

	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	mRasterizer = renderer->createOrGetRasterizer(rasterDesc);
}
