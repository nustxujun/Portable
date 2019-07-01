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
		{
			std::vector<std::string> textures = {
					"media/rustediron/rustediron2_basecolor.png",
					"media/rustediron/rustediron2_normal.png",
					"media/rustediron/rustediron2_roughness.png",
					"media/rustediron/rustediron2_metallic.png",
			};
			Parameters params;
			params["geom"] = "plane";
			params["size"] = "50";
			auto model = mScene->createModel("plane", params, [this](const Parameters& p)
			{
				return Mesh::Ptr(new GeometryMesh(p, mRenderer));
			});
			model->setCastShadow(true);
			model->attach(root);
			Material::Ptr mat = Material::create();
			mat->roughness = 1;
			mat->metallic = 1;
			for (int i = 0; i < textures.size(); ++i)
				if (!textures[i].empty())
					mat->setTexture(i, mRenderer->createTexture(textures[i]));
			model->setMaterial(mat);
		}



		int spherecount = 10;
		for (int i = 0; i < spherecount; ++i)
		{
			std::vector<std::string> textures = {
					"media/streaked/streaked-metal1-albedo.png",
					"",
					"media/streaked/streaked-metal1-rough.png",
					"media/streaked/streaked-metal1-metalness.png",
					"media/streaked/streaked-metal1-ao.png",
			};

			Parameters params;
			params["geom"] = "sphere";
			params["radius"] = "1";
			auto model = mScene->createModel(Common::format("sphere", i), params, [this](const Parameters& p)
			{
				return Mesh::Ptr(new GeometryMesh(p, mRenderer));
			});

			float theta = 3.14159265358  * 2.0f / spherecount;

			float sin = std::sin(i * theta) * 10;
			float cos = std::cos(i* theta) * 10;
			model->getNode()->setPosition({cos , 1, sin });
			model->setCastShadow(true);
			model->attach(root);
			Material::Ptr mat = Material::create();
			mat->roughness = 0.2;

			for (int i = 0; i < textures.size(); ++i)
				if (!textures[i].empty())
					mat->setTexture(i, mRenderer->createTexture(textures[i]));
			model->setMaterial(mat);
		}


		//{
		//	Parameters params;
		//	//params["file"] = "media/model.obj";
		//	params["file"] = "media/sponza/sponza.obj";
		//	auto model = mScene->createModel("test", params, [this](const Parameters& p) {
		//		return Mesh::Ptr(new Mesh(p, mRenderer));
		//	});

		//	model->setCastShadow(true);
		//	model->attach(mScene->getRoot());
		//	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
		//	Matrix mat = Matrix::CreateFromYawPitchRoll(0, -3.14 / 2, 0);
		//	//model->getNode()->setOrientation(Quaternion::CreateFromRotationMatrix(mat));
		//}
		auto aabb = root->getWorldAABB();

		Vector3 vec = aabb.second - aabb.first;

		auto cam = mScene->createOrGetCamera("main");
		DirectX::XMFLOAT3 eye(aabb.second);
		DirectX::XMFLOAT3 at(aabb.first);
		//cam->lookat(eye, at);
		//cam->lookat((aabb.first + aabb.second) * 0.5, { 0,0,0 });
		cam->getNode()->setPosition((aabb.first + aabb.second) * 0.5);
		cam->setDirection({ 0, -1, 0 });
		cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
		cam->setNearFar(1, vec.Length() + 1);
		cam->setFOVy(0.785398185);


		auto light = mScene->createOrGetLight("main");
		light->setDirection({ 0,-1,0 });
		light->setCastingShadow(true);
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

		mPipeline->addShaderResource("irradinace", {});
		mPipeline->addShaderResource("prefiltered", {});
		mPipeline->addShaderResource("lut", {});

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
		mPipeline->pushStage<ShadowMap>(2048, 3, shadowmaps);
		mPipeline->pushStage<PBR>(Vector3(), shadowmaps);
		mPipeline->pushStage<EnvironmentMapping>((aabb.first + aabb.second) * 0.5f, aabb.second - aabb.first, "media/Ditch-River_2k.hdr");

		//mPipeline->pushStage<AO>();
		mPipeline->pushStage<SSR>();
		mPipeline->pushStage<SkyBox>("media/Ditch-River_2k.hdr", false);
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