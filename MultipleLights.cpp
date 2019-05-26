#include "MultipleLights.h"
#include <sstream>
#include "GBuffer.h"
#include "PBR.h"
#include "HDR.h"
#include "PostProcessing.h"
#include "GeometryMesh.h"
#include "DepthBounding.h"
#include "LightCulling.h"
#include "AO.h"

#include <random>

void MultipleLights::initPipeline()
{


	//initDRPipeline();
	initTBDRPipeline();
}

void MultipleLights::initScene()
{
	auto root = mScene->getRoot();
	{
		Parameters params;
		params["geom"] = "room";
		params["size"] = "100";
		auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});
		model->setCastShadow(false);
		model->attach(root);
		model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	}


	{
		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "10";
		auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});
		model->setCastShadow(false);
		model->attach(root);
		model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	
	}

	//{
	//	Parameters params;
	//	//params["file"] = "tiny.x";
	//	params["file"] = "media/sponza/sponza.obj";
	//	auto model = mScene->createModel("test", params, [this](const Parameters& p) {
	//		return Mesh::Ptr(new Mesh(p, mRenderer));
	//	});

	//	model->attach(mScene->getRoot());
	//	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	//
	//}

	auto cam = mScene->createOrGetCamera("main");
	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setFOVy(0.785398185);
	auto aabb = root->getWorldAABB();
	auto len = aabb.second - aabb.first;
	cam->setNearFar(1.0f, (len).Length());

	cam->lookat(aabb.second, aabb.first);
	std::uniform_real_distribution<float> rand(0.0f, 0.1f);
	std::uniform_real_distribution<float> rand2(-1.0f, 1.0f);

	std::default_random_engine gen;

	struct State
	{
		Scene::Light::Ptr light;
		Vector3 vel;
	};
	std::shared_ptr<std::vector<State>> lights = std::shared_ptr<std::vector<State>>(new std::vector<State>());
	int numlights = 2;
	for (int i = 0; i < numlights; ++i)
	{
		std::stringstream ss;
		ss << i;
		auto light = mScene->createOrGetLight(ss.str());
		Vector3 color = { rand(gen),rand(gen),rand(gen) };
		color.Normalize();
		//color *= 1;
		light->setColor(color);
		light->setType(Scene::Light::LT_POINT);
		light->attach(mScene->getRoot());
		float step = 3.14159265358f * 2.0f / (float)numlights;
		float t = i * step;
		float z = sin(t) * 30;
		float x = cos(t) * 30;
		//light->getNode()->setPosition(rand2(gen) *len.x * 0.5f, rand2(gen) * len.y  * 0.5f, rand2(gen) * len.z  * 0.5f);
		//light->getNode()->setPosition( 0.0f, len.y  * 0.5f - 0.1f,0.0f );
		light->getNode()->setPosition(x, 49.0f, z);
		//light->getNode()->setPosition(0.0f, 49.0f, ((i % 2) * 2.0f - 1) * 30.0f);
		light->attach(root);
		Vector3 vel = { rand(gen),rand(gen),rand(gen) };
		vel.Normalize();
		vel *= 0.0f;
		lights->push_back({light,vel });
	}

	//mUpdater = [lights]() {
	//	for(auto&i: *lights)
	//	{ 
	//		auto node = i.light->getNode();
	//		auto pos = node->getPosition() + i.vel;

	//		if (pos.x > 50 || pos.x < -50)
	//			i.vel.x = -i.vel.x;
	//		if (pos.y > 50 || pos.y < -50)
	//			i.vel.y = -i.vel.y;
	//		if (pos.z > 50 || pos.z < -50)
	//			i.vel.z = -i.vel.z;

	//		node->setPosition(pos);
	//	}
	//};


}

void MultipleLights::update()
{
	if (mUpdater)
		mUpdater();
	Framework::update();
}

void MultipleLights::initDRPipeline()
{
	Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
	auto w = mRenderer->getWidth();
	auto h = mRenderer->getHeight();
	auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage([bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>(albedo, normal, worldpos, depth);
	mPipeline->pushStage<PBR>(albedo, normal, depth);
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage([bb, quad](Renderer::Texture::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});
}

void MultipleLights::initTBDRPipeline()
{
	Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
	auto w = mRenderer->getWidth();
	auto h = mRenderer->getHeight();
	auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);


	int bw = ((w + 16 - 1) & ~15) / 16;
	int bh = ((h + 16 - 1) & ~15) / 16;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = bw;
	desc.Height = bh;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	auto depthBounds = mRenderer->createTexture(desc);

	auto lightindex = mRenderer->createRWBuffer(
		desc.Width * desc.Height * (1 + 100) * sizeof(int),
		sizeof(int),
		DXGI_FORMAT_R16_UINT,
		D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage([bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>(albedo, normal, worldpos, depth);
	mPipeline->pushStage<DepthBounding>(depth, depthBounds);
	mPipeline->pushStage<LightCulling>(depthBounds, lightindex);

	mPipeline->pushStage<PBR>(albedo, normal, depth, lightindex);
	//mPipeline->pushStage<AO>(normal,depth, 10);
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage([bb, quad](Renderer::Texture::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});



}
