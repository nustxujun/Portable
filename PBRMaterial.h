#pragma once

#include "Observer.h"
#include <sstream>
#include "DepthLinearing.h"
#include "AO.h"
#include "HDR.h"
#include "PBR.h"
#include "GBuffer.h"
#include "SkyBox.h"

class PBRMaterial :public Observer
{
public:
	using Observer::Observer;
	virtual void initScene() override
	{
		int numpoints = 0;
		int numspots = 0;
		int numdirs = 1;

		set("numdirs", { {"value", numdirs}, {"min", 0}, {"max", numdirs}, {"interval", 1}, {"type","set"} });
		set("dirradiance", { {"type","set"}, {"value",1},{"min","0.1"},{"max",100},{"interval", "0.1"} });

		set("fovy", { {"value", 0.785398185f}, {"min", "0.1"}, {"max", "2"}, {"interval", "0.01"}, {"type","set"} });

		auto root = mScene->getRoot();


		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "1";
		params["resolution"] = "50";
		params["size"] = "1";


		{
			auto model = mScene->createModel("rustediron", params, [this](const Parameters& p)
			{
				return Mesh::Ptr(new GeometryMesh(p, mRenderer));
			});
			model->attach(root);
			model->getNode()->setPosition(0.0f, 0, 0);
			Material::Ptr mat = Material::create();
			mat->setTexture(0, mRenderer->createTexture("media/rustediron/rustediron2_basecolor.png"));
			//mat->setTexture(1, normal);
			mat->setTexture(2, mRenderer->createTexture("media/rustediron/rustediron2_roughness.png"));
			mat->setTexture(3, mRenderer->createTexture("media/rustediron/rustediron2_metallic.png"));
			//mat->setTexture(4, ao);
			model->setMaterial(mat);
		}

		{
			auto model = mScene->createModel("streaked", params, [this](const Parameters& p)
			{
				return Mesh::Ptr(new GeometryMesh(p, mRenderer));
			});
			model->attach(root);
			model->getNode()->setPosition(0.0f, 0.0f, 2.5f);
			Material::Ptr mat = Material::create();
			mat->setTexture(0, mRenderer->createTexture("media/streaked/streaked-metal1-albedo.png"));
			//mat->setTexture(1, normal);
			mat->setTexture(2, mRenderer->createTexture("media/streaked/streaked-metal1-rough.png"));
			mat->setTexture(3, mRenderer->createTexture("media/streaked/streaked-metal1-metalness.png"));
			mat->setTexture(4, mRenderer->createTexture("media/streaked/streaked-metal1-ao.png"));
			model->setMaterial(mat);
		}

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
	}

	virtual void framemove()
	{
		auto lightbuffer = mPipeline->getBuffer("dirlights");
		Vector4 lightinfo[2];
		auto l = mScene->createOrGetLight("main");
		auto pos = l->getNode()->getRealPosition();
		lightinfo[0] = { pos.x, pos.y, pos.z, 0 };
		auto color = l->getColor() * getValue<float>("dirradiance");
		lightinfo[1] = { color.x, color.y, color.z , 0};
		lightbuffer.lock()->blit(lightinfo,sizeof(lightinfo));
	}

	virtual void initPipeline()override
	{
		auto w = mRenderer->getWidth();
		auto h = mRenderer->getHeight();
		auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
		mPipeline->addShaderResource("albedo", albedo);
		mPipeline->addRenderTarget("albedo", albedo);
		auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
		mPipeline->addShaderResource("normal", normal);
		mPipeline->addRenderTarget("normal", normal);
		auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
		mPipeline->addShaderResource("depth", depth);
		mPipeline->addDepthStencil("depth", depth);
		auto depthLinear = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32_FLOAT);
		mPipeline->addShaderResource("depthlinear", depthLinear);
		mPipeline->addRenderTarget("depthlinear", depthLinear);
		auto material = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32_FLOAT);
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

		std::string hdrenvfile = "media/Ditch-River_2k.hdr";
		auto equirect = mRenderer->createTexture(hdrenvfile, 1);
		{
			auto proc = ImageProcessing::create<IrradianceCubemap>(mRenderer, ImageProcessing::RT_TEMP, false);
			auto ret = proc->process(equirect);
			mPipeline->addShaderResource("irradinace", ret);
		}
		{
			auto proc = ImageProcessing::create<PrefilterCubemap>(mRenderer, ImageProcessing::RT_TEMP, false);
			auto ret = proc->process(equirect);
			mPipeline->addShaderResource("prefiltered", ret);
		}
		auto lut = mRenderer->createTexture("media/IBL_BRDF_LUT.png", 1);
		mPipeline->addShaderResource("lut", lut);

		auto bb = mRenderer->getBackbuffer();
		mPipeline->pushStage("clear rt", [bb, this](Renderer::Texture2D::Ptr rt)
		{
			mRenderer->clearRenderTarget(rt, { 0,0,0,0 });
		});

		mPipeline->pushStage<GBuffer>();
		mPipeline->pushStage<DepthLinearing>();
		mPipeline->pushStage<PBR>();
		mPipeline->pushStage<AO>(3.0f);
		mPipeline->pushStage<SkyBox>(hdrenvfile, false);
		mPipeline->pushStage<HDR>();
		mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");
		Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
		mPipeline->pushStage("draw to backbuffer", [bb, quad](Renderer::Texture2D::Ptr rt)
		{
			quad->setRenderTarget(bb);
			quad->drawTexture(rt, false);
		});

	}


};