#pragma once
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
#include "IrradianceVolumes.h"
#include "TAA.h"
#include "MotionVector.h"
#include "Voxelize.h"


#include <random>

class VoxelConeTracing :public Framework
{
public:
	using Framework::Framework;

	virtual void initScene()
	{
		int numdirs = 1;

		set("time", { {"value", 3.14f}, {"min", "1.57"}, {"max", "4.71"}, {"interval", "0.001"}, {"type","set"} });
		//set("numdirs", { {"value", numdirs}, {"min", 0}, {"max", numdirs}, {"interval", 1}, {"type","set"} });
		//set("dirradiance", { {"type","set"}, {"value",100},{"min","0.1"},{"max",100},{"interval", "0.1"} });
		set("lightRange", { {"value", 1000}, {"min", 1}, {"max", 1000}, {"interval", 1}, {"type","set"} });
		set("numpoints", { {"value", 1}, {"min", 0}, {"max", 1}, {"interval", 1}, {"type","set"} });
		set("pointradiance", { {"type","set"}, {"value",1},{"min","0.1"},{"max",100},{"interval", "0.1"} });

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
		//	mat->reflection = 1;
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
		//	model->getNode()->setPosition({ 0,25,25 });
		//	model->getNode()->rotate(-3.1415926  , 0, 0);
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	Material::Ptr mat = Material::create();
		//	mat->roughness = 1;
		//	mat->metallic = 0;
		//	mat->reflection = 0;
		//	mat->diffuse = { 0,1, 0 };
		//	model->setMaterial(mat);


		//	model = mScene->createModel("wall2", params, [this](const Parameters& p)
		//	{
		//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//	});
		//	model->getNode()->setPosition({ 0,25,-25 });
		//	model->getNode()->rotate(3.1415926 , 0, 0);
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	mat = Material::create();
		//	mat->roughness = 1;
		//	mat->metallic = 0;
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
		//	mat->roughness = 1;
		//	mat->metallic = 0;
		//	mat->reflection = 0;
		//	mat->diffuse = { 1,0, 0 };
		//	model->setMaterial(mat);
		//}

		//{
		//	int spherecount = 1;
		//	Parameters params;
		//	params["geom"] = "sphere";
		//	params["radius"] = "1";
		//	params["size"] = "1";
		//	Material::Ptr mat = Material::create();
		//	std::vector<std::string> textures = {
		//	"media/streaked/streaked-metal1-albedo.png",
		//	"",
		//	"media/streaked/streaked-metal1-rough.png",
		//	"media/streaked/streaked-metal1-metalness.png",
		//	"media/streaked/streaked-metal1-ao.png",
		//	};
		//	mat->metallic = 0;
		//	mat->roughness = 1;
		//	//for (int i = 0; i < textures.size(); ++i)
		//	//	if (!textures[i].empty())
		//	//		mat->setTexture(i, mRenderer->createTexture(textures[i]));
		//	auto model = mScene->createModel(Common::format("sphere"), params, [this, mat](const Parameters& p)
		//	{
		//		auto sphere = Mesh::Ptr(new GeometryMesh(p, mRenderer));
		//		sphere->setMaterial(mat);

		//		return sphere;
		//	});


		//	model->getNode()->setPosition({0 , 0, 0 });
		//	model->setCastShadow(true);
		//	model->attach(root);
		//	model->setStatic(false);

		//}

		{
			Parameters params;
			params["file"] = "media/sponza/sponza.obj";
			//params["file"] = "media/CornellBox/CornellBox-Empty-RG.obj";
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
		//cam->lookat({ -20,10,0 }, { 0,0,0 });
		cam->lookat(aabb.first, aabb.second);
		//cam->getNode()->setPosition((aabb.first + aabb.second) * 0.5);
		//cam->setDirection({ 0, -1, 0 });
		cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
		cam->setNearFar(vec.Length() * 0.01f, vec.Length() * 2);
		cam->setFOVy(0.785398185);


		auto light = mScene->createOrGetLight("main");
		light->setCastingShadow(false);
		light->setType(Scene::Light::LT_POINT);
		light->getNode()->setPosition((aabb.first + aabb.second) * 0.5f);

		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "0.1";
		auto sphere = Mesh::Ptr(new GeometryMesh(params, mRenderer));
		sphere->getMesh(0).material->roughness = 0;
		sphere->getMesh(0).material->metallic = 1;

		auto probe = mScene->createProbe("probe");
		probe->getNode()->setPosition((aabb.first + aabb.second)*0.5f);
		//probe->setDebugObject(sphere);
		probe->setProxy(Scene::Probe::PP_DEPTH);
		probe->setType(Scene::Probe::PT_SPECULAR);
		probe->attach(root);

	}


	void initPipeline()
	{
		auto cam = mScene->createOrGetCamera("main");
		mPipeline->setCamera(cam);
		auto vp = cam->getViewport();
		auto w = vp.Width;
		auto h = vp.Height;


		auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
		mPipeline->setFrameBuffer(frame);


		constexpr auto MAX_SHADOWMAPS = 1;
		std::vector<Renderer::Texture2D::Ptr> shadowmaps;
		for (int i = 0; i < MAX_SHADOWMAPS; ++i)
			shadowmaps.push_back(mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32_FLOAT));

		auto bb = mRenderer->getBackbuffer();
		mPipeline->pushStage("clear rt", [bb, this](Renderer::Texture2D::Ptr rt)
		{
			mRenderer->clearRenderTarget(rt, { 0,0,0,0 });
		});



		mPipeline->pushStage<GBuffer>(true, [](Scene::Entity::Ptr e)
		{
			return e->getMask() == 1;
			//return true;
		});
		mPipeline->pushStage<PBR>(Vector3(), shadowmaps);
		mPipeline->pushStage<AO>();
		mPipeline->pushStage<Voxelize>(256);

		mPipeline->pushStage<SkyBox>("media/white.png", false);


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

		auto cam = mScene->createOrGetCamera("main");
		auto aabb = mScene->getRoot()->getWorldAABB();
		auto len = (aabb.second - aabb.first).Length();
		cam->setNearFar(len * 0.01f, len * 2);
		//auto model = mScene->getModel("sphere");
		//if (model)
		//{
		//	float time = GetTickCount() * 0.001;
		//	float sin = std::sin(time) * 0.5;
		//	float cos = std::cos(time) * 0.5;
		//	model->getNode()->setPosition(cos, cos + 1, sin);
		//}
	}

};