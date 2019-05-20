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
	mPipeline->pushStage<PBR>(albedo, normal, depth, 0.5f, 0.9f);
	mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage([bb, quad](Renderer::RenderTarget::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});
}

void MultipleLights::initScene()
{
	auto root = mScene->getRoot();
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

	auto cam = mScene->createOrGetCamera("main");
	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setFOVy(0.785398185);
	auto aabb = root->getWorldAABB();
	cam->setNearFar(1.0f, (aabb.second - aabb.first).Length());

	cam->lookat(aabb.second, aabb.first);
	std::uniform_real_distribution<float> rand(0.0f, 0.1f);
	std::default_random_engine gen;


	std::vector<Scene::Light::Ptr> lights;
	for (int i = 0; i < 5; ++i)
	{
		std::stringstream ss;
		ss << i;
		auto light = mScene->createOrGetLight(ss.str());
		//Vector3 color = { rand(gen),rand(gen),rand(gen) };
		Vector3 color = { 1,1 ,1 };
		color *= 0.1f;
		light->setColor(color);
		light->setType(Scene::Light::LT_POINT);
		light->attach(mScene->getRoot());
		light->getNode()->setPosition((float)i * 10 - 30, 49.0f, 0.0f);
		light->attach(root);
		lights.push_back(light);
	}





}
