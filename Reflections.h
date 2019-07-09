#pragma once

#include "Framework.h"
#include "GeometryMesh.h"

#include "GBuffer.h"
#include "PBR.h"
#include "ShadowMap.h"
#include "HDR.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "SSR.h"
#include "EnvironmentMapping.h"
#include "AO.h"
#include "MotionBlur.h"

#include <random>

class Reflections :public Framework
{
public:
	using Framework::Framework;

	virtual void initScene()
	{
		int numdirs = 1;

		set("time", { {"value", 3.14f}, {"min", "1.57"}, {"max", "4.71"}, {"interval", "0.001"}, {"type","set"} });
		set("numdirs", { {"value", numdirs}, {"min", 0}, {"max", numdirs}, {"interval", 1}, {"type","set"} });
		set("dirradiance", { {"type","set"}, {"value",1},{"min","0.1"},{"max",100},{"interval", "0.1"} });

		auto root = mScene->getRoot();
		//{
		//	std::vector<std::string> textures = {
		//			"media/rustediron/rustediron2_basecolor.png",
		//			"media/rustediron/rustediron2_normal.png",
		//			"media/rustediron/rustediron2_roughness.png",
		//			"media/rustediron/rustediron2_metallic.png",
		//	};
		//	Parameters params;
		//	params["geom"] = "plane";
		//	params["size"] = "50";
		//	auto model = mScene->createModel("plane", params, [this](const Parameters& p)
		//	{
		//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//	});
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	Material::Ptr mat = Material::create();
		//	mat->roughness = 1;
		//	mat->metallic = 1;
		//	for (int i = 0; i < textures.size(); ++i)
		//		if (!textures[i].empty())
		//			mat->setTexture(i, mRenderer->createTexture(textures[i]));
		//	model->setMaterial(mat);
		//}

		//{
		//	Parameters params;
		//	params["geom"] = "plane";
		//	params["size"] = "50";
		//	auto model = mScene->createModel("wall1", params, [this](const Parameters& p)
		//	{
		//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//	});
		//	model->getNode()->setPosition({ 0,25,24 });
		//	model->getNode()->rotate( -3.1415926 * 0.5, 0, 0);
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	Material::Ptr mat = Material::create();
		//	mat->roughness = 0;
		//	mat->metallic = 1;
		//	mat->reflection = 0;
		//	mat->diffuse = { 0,1, 0};
		//	model->setMaterial(mat);


		//	model = mScene->createModel("wall2", params, [this](const Parameters& p)
		//	{
		//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//	});
		//	model->getNode()->setPosition({ 0,25,-24 });
		//	model->getNode()->rotate(3.1415926 * 0.5, 0, 0);
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	mat = Material::create();
		//	mat->roughness = 0;
		//	mat->metallic = 1;
		//	mat->reflection = 0;
		//	mat->diffuse = { 0,0, 1 };
		//	model->setMaterial(mat);

		//	model = mScene->createModel("wall3", params, [this](const Parameters& p)
		//	{
		//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//	});
		//	model->getNode()->setPosition({ 24,25,0 });
		//	model->getNode()->rotate(0, 0, 3.1415926 * 0.5);
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	mat = Material::create();
		//	mat->roughness = 0;
		//	mat->metallic = 1;
		//	mat->reflection = 0;
		//	mat->diffuse = { 1,0, 0 };
		//	model->setMaterial(mat);
		//}

		//{
		//	int spherecount = 10;
		//	Parameters params;
		//	params["geom"] = "sphere";
		//	params["radius"] = "1";
		//	Material::Ptr mat = Material::create();
		//	mat->reflection = 0;
		//	std::vector<std::string> textures = {
		//	"media/streaked/streaked-metal1-albedo.png",
		//	"",
		//	"media/streaked/streaked-metal1-rough.png",
		//	"media/streaked/streaked-metal1-metalness.png",
		//	"media/streaked/streaked-metal1-ao.png",
		//	};
		//	for (int i = 0; i < textures.size(); ++i)
		//		if (!textures[i].empty())
		//			mat->setTexture(i, mRenderer->createTexture(textures[i]));
		//	for (int i = 0; i < spherecount; ++i)
		//	{
		//		auto model = mScene->createModel(Common::format("sphere", i), params, [this, mat](const Parameters& p)
		//		{
		//			auto sphere = Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//			sphere->setMaterial(mat);

		//			return sphere;
		//		});

		//		float theta = 3.14159265358  * 2.0f / spherecount;

		//		float sin = std::sin(i * theta) * 4;
		//		float cos = std::cos(i* theta) * 10;
		//		model->getNode()->setPosition({ cos , cos * 0.5f + 6, sin });
		//		model->setCastShadow(true);
		//		model->attach(root);
		//		model->setStatic(false);
		//	}
		//}													

		{
			Parameters params;
			//params["file"] = "media/model.obj";
			params["file"] = "media/sponza/sponza.obj";
			auto model = mScene->createModel("test", params, [this](const Parameters& p) {
				return Mesh::Ptr(new Mesh(p, mRenderer));
			});

			model->setCastShadow(true);
			model->attach(mScene->getRoot());
			model->getNode()->setPosition(0.0f, 0.f, 0.0f);
			Matrix mat = Matrix::CreateFromYawPitchRoll(0, -3.14 / 2, 0);
			//model->getNode()->setOrientation(Quaternion::CreateFromRotationMatrix(mat));
		}
		auto aabb = root->getWorldAABB();

		Vector3 vec = aabb.second - aabb.first;

		auto cam = mScene->createOrGetCamera("main");
		DirectX::XMFLOAT3 eye(aabb.second);
		DirectX::XMFLOAT3 at(aabb.first);
		cam->lookat({ -20,10,0 }, {0,0,0});
		//cam->lookat((aabb.first + aabb.second) * 0.5, { 0,0,0 });
		//cam->getNode()->setPosition((aabb.first + aabb.second) * 0.5);
		//cam->setDirection({ 0, -1, 0 });
		cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
		cam->setNearFar(1, vec.Length() + 1);
		cam->setFOVy(0.785398185);


		auto light = mScene->createOrGetLight("main");
		light->setDirection({ 0,-1,0 });
		light->setCastingShadow(true);


		int numprobes = 10;
		auto half = vec * 0.5f;
		auto dx = vec.x / numprobes;
		auto pos = (aabb.first + aabb.second) * 0.5f;
		std::uniform_real_distribution<float> rand(0.0f, 1.0f);
		std::default_random_engine gen;
		
		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "1";
		auto sphere = Mesh::Ptr(new GeometryMesh(params, mRenderer));
		sphere->getMesh(0).material->roughness = 0;
		for (int i = 0; i < numprobes; ++i)
		{
			auto probe = mScene->createProbe(Common::format(i));
			probe->setDebugObject(sphere);
			Vector3 offset = { dx * 0.5f + i * dx - half.x, 0,0 };

			probe->setProjectionBox(Vector3(vec.x, vec.y, vec.z), offset);
			//probe->setInfluence(Vector3(dx , vec.y + 1, vec.z));
			probe->setInfluence(vec + Vector3(0,1,0), offset);
			probe->getNode()->setPosition(-offset + Vector3(pos.x, pos.y, pos.z));
			probe->setColor({0, float(i % 2), 1.0f - i % 2});
			probe->attach(root);

		}
	}


	void initPipeline()
	{
		auto cam = mScene->createOrGetCamera("main");
		mPipeline->setCamera(cam);
		auto vp = cam->getViewport();
		auto w = vp.Width;
		auto h = vp.Height;
		auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
		mPipeline->addShaderResource("albedo", albedo);
		mPipeline->addRenderTarget("albedo", albedo);
		auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
		mPipeline->addShaderResource("normal", normal);
		mPipeline->addRenderTarget("normal", normal);
		auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R24G8_TYPELESS, true);
		mPipeline->addShaderResource("depth", depth);
		mPipeline->addDepthStencil("depth", depth);
		mPipeline->addTexture2D("depth", depth);
		auto material = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
		mPipeline->addShaderResource("material", material);
		mPipeline->addRenderTarget("material", material);

		constexpr auto MAX_NUM_LIGHTS = 1024;
		auto pointlights = mRenderer->createRWBuffer(sizeof(Vector4) * 2 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		mPipeline->addShaderResource("pointlights", pointlights);
		mPipeline->addBuffer("pointlights", pointlights);

		auto spotlights = mRenderer->createRWBuffer(sizeof(Vector4) * 3 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		mPipeline->addShaderResource("spotlights", spotlights);
		mPipeline->addBuffer("spotlights", spotlights);

		auto dirlights = mRenderer->createRWBuffer(sizeof(Vector4) * 2 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		mPipeline->addShaderResource("dirlights", dirlights);
		mPipeline->addBuffer("dirlights", dirlights);

		auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
		mPipeline->setFrameBuffer(frame);


		constexpr auto MAX_SHADOWMAPS = 1;
		std::vector<Renderer::Texture2D::Ptr> shadowmaps;
		for (int i = 0; i < MAX_SHADOWMAPS; ++i)
			shadowmaps.push_back(mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32_FLOAT));

		auto bb = mRenderer->getBackbuffer();
		mPipeline->pushStage("clear rt", [bb, this, depth](Renderer::Texture2D::Ptr rt)
		{
			mRenderer->clearRenderTarget(rt, { 0,0,0,0 });
			mRenderer->clearDepth(depth, 1.0f);
		});


		auto aabb = mScene->getRoot()->getWorldAABB();

		mPipeline->pushStage<GBuffer>();
		//mPipeline->pushStage<ShadowMap>(2048, 3, shadowmaps);
		mPipeline->pushStage<PBR>(Vector3(), shadowmaps);
		mPipeline->pushStage<EnvironmentMapping>(EnvironmentMapping::T_ONCE, "media/Ditch-River_2k.hdr");

		//mPipeline->pushStage<AO>();
		//mPipeline->pushStage<SSR>();
		mPipeline->pushStage<SkyBox>("media/Ditch-River_2k.hdr", false);
		//mPipeline->pushStage<MotionBlur>(true);
		mPipeline->pushStage<HDR>();
		mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");
		Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
		mPipeline->pushStage("draw to backbuffer", [bb, quad](Renderer::Texture2D::Ptr rt)
		{
			quad->setRenderTarget(bb);
			quad->drawTexture(rt, false);
		});

	}


	void framemove()
	{
		Framework::framemove();
		float time = GetTickCount() * 0.005f;
		float d = 3.1415926f * 2.0f / 10;
		for (int i = 0; i < 10; ++i)
		{
			auto s = mScene->getModel(Common::format("sphere", i));
			if (!s)
				continue;
			float sin = std::sin(time + d * i) * 10;
			float cos = std::cos(time + d * i) * 10;
			float sin2 = std::sin(time + i * i + d * i) * 10;
			s->getNode()->setPosition(cos, sin2 + 10 , sin);
		}
	}

};