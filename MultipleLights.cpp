#include "MultipleLights.h"
#include <sstream>
#include "GBuffer.h"
#include "PBR.h"
#include "HDR.h"
#include "PostProcessing.h"
#include "GeometryMesh.h"
#include <random>

void MultipleLights::initPipeline()
{
	initDRPipeline();
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


	auto cam = mScene->createOrGetCamera("main");
	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setFOVy(0.785398185);
	auto aabb = root->getWorldAABB();
	cam->setNearFar(1.0f, (aabb.second - aabb.first).Length());

	cam->lookat(aabb.second, aabb.first);
	std::uniform_real_distribution<float> rand(0.0f, 0.1f);
	std::default_random_engine gen;

	struct State
	{
		Scene::Light::Ptr light;
		Vector3 vel;
	};
	std::shared_ptr<std::vector<State>> lights = std::shared_ptr<std::vector<State>>(new std::vector<State>());
	int numlights = 100;
	for (int i = 0; i < numlights; ++i)
	{
		std::stringstream ss;
		ss << i;
		auto light = mScene->createOrGetLight(ss.str());
		Vector3 color = { rand(gen),rand(gen),rand(gen) };
		color.Normalize();
		color *= 50;
		light->setColor(color);
		light->setType(Scene::Light::LT_POINT);
		light->attach(mScene->getRoot());
		float step = 3.14159265358f * 2.0f / (float)numlights;
		float t = i * step;
		float z = sin(t) * 30;
		float x = cos(t) * 30;
		light->getNode()->setPosition(x, 49.0f, z);
		light->attach(root);
		Vector3 vel = { rand(gen),rand(gen),rand(gen) };
		vel.Normalize();
		vel *= 1.0f;
		lights->push_back({light,vel });
	}

	mUpdater = [lights]() {
		for(auto&i: *lights)
		{ 
			auto node = i.light->getNode();
			auto pos = node->getPosition() + i.vel;

			if (pos.x > 50 || pos.x < -50)
				i.vel.x = -i.vel.x;
			if (pos.y > 50 || pos.y < -50)
				i.vel.y = -i.vel.y;
			if (pos.z > 50 || pos.z < -50)
				i.vel.z = -i.vel.z;

			node->setPosition(pos);
		}
	};


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
	mPipeline->pushStage<PBR>(albedo, normal, depth, 0.9f, 0.1f);
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage([bb, quad](Renderer::RenderTarget::Ptr rt)
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

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage([bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>(albedo, normal, worldpos, depth);
	mPipeline->pushStage<PBR>(albedo, normal, depth, 0.1f, 0.9f);
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage([bb, quad](Renderer::RenderTarget::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});
}
