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
#include "DepthLinearing.h"
#include "ClusteredLightCulling.h"

#include <random>

void MultipleLights::initPipeline()
{

	//initDRPipeline();
	//initTBDRPipeline();
	initCDRPipeline();
}

void MultipleLights::initScene()
{
	int numlights = 1000;


	set("numLights", { {"value", 100}, {"min", 1}, {"max", numlights}, {"interval", 1}, {"type","set"} });
	set("lightRange", { {"value", 500}, {"min", 1}, {"max", 1000}, {"interval", 1}, {"type","set"} });
	set("fovy", { {"value", "0.7"}, {"min", "0.1"}, {"max", "2"}, {"interval", "0.01"}, {"type","set"} });

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
		params["geom"] = "cube";
		params["size"] = "101";
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
		params["resolution"] = "500";
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
	//	params["file"] = "lost-empire/lost_empire.obj";
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

	cam->getNode()->setPosition((aabb.second + aabb.first) * 0.5f);
	cam->setDirection({ 0,0,1 });
	//cam->lookat((aabb.second + aabb.first) * 0.5f, aabb.first);
	std::uniform_real_distribution<float> rand(0.0f, 0.1f);
	std::uniform_real_distribution<float> rand2(-1.0f, 1.0f);

	std::default_random_engine gen;

	struct State
	{
		Scene::Light::Ptr light;
		Vector3 vel;
	};
	std::shared_ptr<std::vector<State>> lights = std::shared_ptr<std::vector<State>>(new std::vector<State>());
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
		light->getNode()->setPosition(rand2(gen) *len.x * 0.5f, rand2(gen) * len.y  * 0.5f, rand2(gen) * len.z  * 0.5f);
		//light->getNode()->setPosition( 0.0f, len.y  * 0.5f - 0.1f,0.0f );
		//light->getNode()->setPosition(x, 49.0f, z);
		//light->getNode()->setPosition(0.0f, 49.0f, ((i % 2) * 2.0f - 1) * 30.0f);
		light->attach(root);
		Vector3 vel = { rand(gen),rand(gen),rand(gen) };
		vel.Normalize();
		vel *= 1.0f;
		lights->push_back({light,vel });
	}

	//mUpdater = [lights,len]() {
	//	for(auto&i: *lights)
	//	{ 
	//		auto node = i.light->getNode();
	//		auto pos = node->getPosition() + i.vel;

	//		if (pos.x > len.x || pos.x < -len.x)
	//			i.vel.x = -i.vel.x;
	//		if (pos.y > len.y || pos.y < -len.y)
	//			i.vel.y = -i.vel.y;
	//		if (pos.z > len.z || pos.z < -len.z)
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
	mPipeline->addShaderResource("albedo", albedo);
	mPipeline->addRenderTarget("albedo", albedo);
	auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	mPipeline->addShaderResource("normal", normal);
	mPipeline->addRenderTarget("normal", normal);
	auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mPipeline->addShaderResource("worldpos", worldpos);
	mPipeline->addRenderTarget("worldpos", worldpos);
	auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
	mPipeline->addShaderResource("depth", depth);
	mPipeline->addDepthStencil("depth", depth);
	auto depthLinear = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32_FLOAT);
	mPipeline->addShaderResource("depthlinear", depthLinear);
	mPipeline->addRenderTarget("depthlinear", depthLinear);
	auto lights = mRenderer->createRWBuffer(sizeof(Vector4) * 2 * mScene->getNumLights(), sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mPipeline->addShaderResource("lights", lights);
	mPipeline->addBuffer("lights", lights);

	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage("clear rt",[bb,depth](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
		depth.lock()->clearDepth(1.0f);
		depth.lock()->clearStencil(1);
	});

	mPipeline->pushStage<GBuffer>();
	mPipeline->pushStage<DepthLinearing>();
	mPipeline->pushStage<PBR>();
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage("draw to backbuffer",[bb, quad](Renderer::Texture::Ptr rt)
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
	mPipeline->addShaderResource("albedo", albedo);
	mPipeline->addRenderTarget("albedo", albedo);
	auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	mPipeline->addShaderResource("normal", normal);
	mPipeline->addRenderTarget("normal", normal);
	auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mPipeline->addShaderResource("worldpos", worldpos);
	mPipeline->addRenderTarget("worldpos", worldpos);
	auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
	mPipeline->addShaderResource("depth", depth);
	mPipeline->addDepthStencil("depth", depth);
	auto depthLinear = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32_FLOAT);
	mPipeline->addShaderResource("depthlinear", depthLinear);
	mPipeline->addRenderTarget("depthlinear", depthLinear);
	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto lights = mRenderer->createRWBuffer(sizeof(Vector4) * 2 * mScene->getNumLights(), sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mPipeline->addShaderResource("lights", lights);
	mPipeline->addBuffer("lights", lights);

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
	mPipeline->addUnorderedAccess("depthbounds", depthBounds);
	mPipeline->addShaderResource("depthbounds", depthBounds);

	auto lightsindex = mRenderer->createRWBuffer(
		desc.Width * desc.Height * (1 + mScene->getNumLights()) * sizeof(int),
		sizeof(int),
		DXGI_FORMAT_R16_UINT,
		D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
	mPipeline->addShaderResource("lightsindex", lightsindex);
	mPipeline->addUnorderedAccess("lightsindex", lightsindex);

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage("clear rt",[bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>();
	mPipeline->pushStage<DepthLinearing>();
	mPipeline->pushStage<DepthBounding>(bw, bh);
	mPipeline->pushStage<LightCulling>(bw, bh);

	mPipeline->pushStage<PBR>();
	//mPipeline->pushStage<AO>(normal,depth, 10);
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage("draw to backbuffer",[bb, quad](Renderer::Texture::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});



}

void MultipleLights::initCDRPipeline()
{
	auto w = mRenderer->getWidth();
	auto h = mRenderer->getHeight();
	auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mPipeline->addShaderResource("albedo", albedo);
	mPipeline->addRenderTarget("albedo", albedo);
	auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	mPipeline->addShaderResource("normal", normal);
	mPipeline->addRenderTarget("normal", normal);
	auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mPipeline->addShaderResource("worldpos", worldpos);
	mPipeline->addRenderTarget("worldpos", worldpos);
	auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
	mPipeline->addShaderResource("depth", depth);
	mPipeline->addDepthStencil("depth", depth);
	auto depthLinear = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32_FLOAT);
	mPipeline->addShaderResource("depthlinear", depthLinear);
	mPipeline->addRenderTarget("depthlinear", depthLinear);
	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto lights = mRenderer->createRWBuffer(sizeof(Vector4) * 2 * mScene->getNumLights(), sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mPipeline->addShaderResource("lights", lights);
	mPipeline->addBuffer("lights", lights);


	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage("clear rt", [bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>();
	mPipeline->pushStage<DepthLinearing>();

	mPipeline->pushStage<ClusteredLightCulling>();

	mPipeline->pushStage<PBR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");
	Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
	mPipeline->pushStage("draw to backbuffer", [bb, quad](Renderer::Texture::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});

}

void MultipleLights::onChanged(const std::string& key, const nlohmann::json::value_type& value)
{
	if (key == "fovy")
	{
		auto cam = mScene->createOrGetCamera("main");
		cam->setFOVy(value["value"]);
	}
}
