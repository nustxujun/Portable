#pragma once

#include "Observer.h"
#include <sstream>
#include "AO.h"
#include "HDR.h"
#include "PBR.h"
#include "GBuffer.h"
#include "SkyBox.h"
#include "SSR.h"
#include "EnvironmentMapping.h"
#include "TAA.h";
#include "MotionVector.h"

class PBRMaterial :public Observer
{
public:
	using Observer::Observer;
	
	void differentMaterials()
	{

		auto root = mScene->getRoot();


		std::vector<std::array<std::string, 6>> materials = 
		{
			{
				"media/rustediron/rustediron2_basecolor.png",
				"media/rustediron/rustediron2_normal.png",
				"media/rustediron/rustediron2_roughness.png",
				"media/rustediron/rustediron2_metallic.png",
				"",
			},
			{
				"media/streaked/streaked-metal1-albedo.png",
				"",
				"media/streaked/streaked-metal1-rough.png",
				"media/streaked/streaked-metal1-metalness.png",
				"media/streaked/streaked-metal1-ao.png",
			},
			{
				"media/burlap/worn-blue-burlap-albedo.png",
				"media/burlap/worn-blue-burlap-Normal-dx.png",
				"media/burlap/worn-blue-burlap-Roughness.png",
				"media/burlap/worn-blue-burlap-Metallic.png",
				"media/burlap/worn-blue-burlap-ao.png",
				"media/burlap/worn-blue-burlap-Height.png"
			},
			{
				"media/foamgrip/foam-grip1-albedo.png",
				"media/foamgrip/foam-grip1-normal-dx1.png",
				"media/foamgrip/foam-grip1-roughness.png",
				"media/foamgrip/foam-grip1-metallic.png",
				"media/foamgrip/foam-grip1-ao.png",
				"media/foamgrip/foam-grip1-height.png"
			},
			{
				"media/gravel/gravel_path-albedo.png",
				"media/gravel/gravel_path_Normal-dx.png",
				"media/gravel/gravel_path_Roughness.png",
				"media/gravel/gravel_path_Metallic.png",
				"media/gravel/gravel_path-ao.png",
				"media/gravel/gravel_path_Height.png"
			},
			{
				"media/rock/holey-rock1-albedo.png",
				"media/rock/holey-rock1-normal-ue.png",
				"media/rock/holey-rock1-roughness.png",
				"media/rock/holey-rock1-metalness.png",
				"media/rock/holey-rock1-ao.png",
				//"media/rock/holey-rock1-height.png"
			},
			{
				"media/bark/bark1-albedo.png",
				"media/bark/bark1-normal3.png",
				"media/bark/bark1-rough.png",
				"media/bark/bark1-metalness.png",
				"media/bark/bark1-ao.png",
				"media/bark/bark1-height2.png"
			},
			{
				"media/wood/bamboo-wood-semigloss-albedo.png",
				"media/wood/bamboo-wood-semigloss-normal.png",
				"media/wood/bamboo-wood-semigloss-roughness.png",
				"media/wood/bamboo-wood-semigloss-metal.png",
				"media/wood/bamboo-wood-semigloss-ao.png",
			},
			{
				"media/rustypanel/rusty-panel-albedo3b.png",
				"media/rustypanel/rusty-panel-norma-dx.png",
				"media/rustypanel/rusty-panel-roughness3b.png",
				"media/rustypanel/rusty-panel-metalness3b.png",
				"media/rustypanel/rusty-panel-ao3.png",
			},
		};

		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "1";
		params["resolution"] = "50";
		params["size"] = "1";

		int col = std::ceil(std::sqrt(materials.size()));
		float z = 0;
		float y = 0;
		int index = 0;

		for (auto& m:materials)
		{
			auto model = mScene->createModel(m[0], params, [this](const Parameters& p)
			{
				return Mesh::Ptr(new GeometryMesh(p, mRenderer));
			});
			model->attach(root);
			z = index % col * 2.5f;
			y = index / col * 2.5f;

			model->getNode()->setPosition(0, y, z);
			index++;
			
			Material::Ptr mat = Material::create();
			mat->roughness = 1;
			mat->metallic = 1;
			for (size_t t = 0; t < m.size(); ++t)
				if (m[t] != "")
					mat->setTexture(t, mRenderer->createTexture(m[t]));
			model->setMaterial(mat);
		}


	}

	void roughAndMetal()
	{
		auto root = mScene->getRoot();

		for (int r = 0; r <= 10; ++r)
		{
			for (int m = 0; m <= 10; ++m)
			{
				Parameters params;
				params["geom"] = "sphere";
				params["radius"] = "1";
				params["resolution"] = "20";
				params["size"] = "1";
				std::stringstream ss;
				ss << "material" << r << m;
				auto model = mScene->createModel(ss.str(), params, [this](const Parameters& p)
				{
					return Mesh::Ptr(new GeometryMesh(p, mRenderer));
				});
				model->setCastShadow(false);
				model->attach(root);
				model->getNode()->setPosition(10.0f, 2.0f * r - 7, 2.0f * m - 7);
				Material::Ptr mat = Material::create();
				mat->metallic = m * 0.1f;
				mat->roughness = r * 0.1f;
				model->setMaterial(mat);
			}
		}
	}

	void plane()
	{
		auto root = mScene->getRoot();

		std::vector<std::string> textures = {
			"media/rock/holey-rock1-albedo.png",
			"media/rock/holey-rock1-normal-ue.png",
			"media/rock/holey-rock1-roughness.png",
			"media/rock/holey-rock1-metalness.png",
			"media/rock/holey-rock1-ao.png",
			"media/rock/holey-rock1-height.png"
				//"media/rustediron/rustediron2_normal.png",

		};


		Parameters params;
		params["geom"] = "plane";
		params["size"] = "10";
		auto model = mScene->createModel("plane", params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});
		model->setCastShadow(false);
		model->attach(root);
		Material::Ptr mat = Material::create();
		for (int i = 0; i < textures.size(); ++i)
			mat->setTexture(i, mRenderer->createTexture(textures[i]));
		model->setMaterial(mat);
	}

	void gun()
	{
		{
			Parameters params;
			params["file"] = "Cerberus/Cerberus.fbx";
			auto model = mScene->createModel("test", params, [this](const Parameters& p) {
				return Mesh::Ptr(new Mesh(p, mRenderer));
			});

			model->attach(mScene->getRoot());
			model->getNode()->setPosition(0.0f, 0.f, 0.0f);
			model->getNode()->setOrientation(Quaternion::CreateFromYawPitchRoll(0, 3.1415926 * 0.5f, 0));


			auto m = model->getMesh()->getMesh(0).material;
			m->setTexture(1, mRenderer->createTexture("Cerberus/Textures/Cerberus_N.tga"));
			m->setTexture(2, mRenderer->createTexture("Cerberus/Textures/Cerberus_R.tga"));
			m->setTexture(3, mRenderer->createTexture("Cerberus/Textures/Cerberus_M.tga"));
			m->setTexture(4, mRenderer->createTexture("Cerberus/Textures/Raw/Cerberus_AO.tga"));
		}
	}
	
	virtual void initScene() override
	{
		int numpoints = 0;
		int numspots = 0;
		int numdirs = 1;

		set("numdirs", { {"value", numdirs}, {"min", 0}, {"max", numdirs}, {"interval", 1}, {"type","set"} });
		set("dirradiance", { {"type","set"}, {"value",1},{"min",0.1},{"max",100},{"interval", "0.1"} });

		auto root = mScene->getRoot();

		//plane();
		differentMaterials();
		//roughAndMetal();
		//gun();

		auto cam = mScene->createOrGetCamera("main");
		cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
		cam->setFOVy(0.785398185f);
		auto aabb = root->getWorldAABB();
		auto half = (aabb.second - aabb.first) * 0.5f;
		auto center = (aabb.second + aabb.first) * 0.5f;
		cam->setNearFar(1.0f, half.Length() * 4);
		//cam->setProjectType(Scene::Camera::PT_ORTHOGRAPHIC);

		cam->lookat(center - Vector3(0,0,half.z), center);

		auto mainlight = mScene->createOrGetLight("main");
		//mainlight->setColor({ 0.9f, 0.7f, 0.4f });
		mainlight->setDirection({ 0,-1,0.1f });
		mainlight->setType(Scene::Light::LT_DIR);
		mainlight->setCastingShadow(false);

		auto probe = mScene->createProbe("main");
		probe->attach(root);
		probe->setType(Scene::Probe::PT_IBL);
		probe->setProxy(Scene::Probe::PP_NONE);
	}

	virtual void framemove()
	{

	}

	virtual void initPipeline()override
	{
		auto cam = mScene->createOrGetCamera("main");
		mPipeline->setCamera(cam);
		auto vp = cam->getViewport();
		auto w = vp.Width;
		auto h = vp.Height;

		auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
		mPipeline->setFrameBuffer(frame);
		
		std::string hdrenvfile = "media/Ditch-River_Env.hdr";

		auto bb = mRenderer->getBackbuffer();
		mPipeline->pushStage("clear rt", [bb, this](Renderer::Texture2D::Ptr rt)
		{
			mRenderer->clearRenderTarget(rt, { 0,0,0,0 });
		});

		mPipeline->pushStage<GBuffer>(true);
		mPipeline->pushStage<MotionVector>();
		mPipeline->pushStage<PBR>();
		//mPipeline->pushStage<AO>(3.0f);
		//mPipeline->pushStage<SSR>();
		mPipeline->pushStage<EnvironmentMapping>(std::string( "media/Ditch-River_2k.hdr" ));
		mPipeline->pushStage<SkyBox>(hdrenvfile, false);
		mPipeline->pushStage<HDR>();
		mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");
		mPipeline->pushStage<TAA>();
		Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
		mPipeline->pushStage("draw to backbuffer", [bb, quad](Renderer::Texture2D::Ptr rt)
		{
			quad->setRenderTarget(bb);
			quad->drawTexture(rt, false);
		});
	}


};