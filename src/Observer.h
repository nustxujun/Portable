#pragma once

#include "Framework.h"
#include "GBuffer.h"
#include "PBR.h"
#include "GeometryMesh.h"
#include"MathUtilities.h"
#include "ShadowMap.h"
#include "HDR.h"
#include "VolumetricLighting.h"
#include "AO.h"
#include "PostProcessing.h"
#include <sstream>
class Observer: public Framework 
{
public :
	using Framework::Framework;

	virtual void initScene()override
	{

		Parameters params;
		////params["file"] = "tiny.x";
		//params["file"] = "media/sponza/sponza.obj";
		//auto model = mScene->createModel("test", params, [this](const Parameters& p) {
		//	return Mesh::Ptr(new Mesh(p, mRenderer));
		//});


		auto node = mScene->createNode("main");
		mScene->getRoot()->addChild(node);
		params["geom"] = "sphere";
		params["radius"] = "1";

		for (int i = 0; i < 10; ++i)
		{
			std::stringstream ss;
			ss << i;
			auto model = mScene->createModel(ss.str(), params, [this](const Parameters& p)
			{
				return Mesh::Ptr(new GeometryMesh(p, mRenderer));
			});

			model->attach(node);
			model->getNode()->setPosition(i * 4.0f, 2.f, 0.0f);
		}

		params["geom"] = "plane";
		params["size"] = "100";
		auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});
		model->setCastShadow(false);
		model->attach(node);
		model->getNode()->setPosition(0.0f, 0.f, 0.0f);

		auto cam = mScene->createOrGetCamera("main");
		cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
		cam->setFOVy(0.785398185);

		auto light = mScene->createOrGetLight("main");
		light->setDirection({ 0.5,-1,0.5 });
		light->setColor({10,0,0});
		light->setType(Scene::Light::LT_POINT);
		light->attach(mScene->getRoot());

		params["geom"] = "sphere";
		params["radius"] = "1";
		model = mScene->createModel("light", params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});

		model->attach(light->getNode());

		light = mScene->createOrGetLight("head");
		light->setDirection({ 0, -1, 0 });
		light->setColor({ 0,1,0 });

	}

	virtual void initPipeline() override
	{
		mPipeline->setCamera(mScene->createOrGetCamera("main"));

		//Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
		//auto w = mRenderer->getWidth();
		//auto h = mRenderer->getHeight();
		//auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
		//auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
		//auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
		//auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS, true);
		//auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);

		//mPipeline->setFrameBuffer(frame);
		//auto bb = mRenderer->getBackbuffer();
		//mPipeline->pushStage("clear rt", [bb](Renderer::RenderTarget::Ptr rt)
		//{
		//	rt.lock()->clear({ 0,0,0,0 });
		//});

		//mPipeline->pushStage<GBuffer>(albedo, normal, worldpos, depth);
		////mPipeline->pushStage<PBR>(albedo, normal, depth);
		////mPipeline->pushStage<AO>(normal, depth,10.0f);
		////mPipeline->pushStage<ShadowMap>(worldpos,depth, 2048, 8);
		////mPipeline->pushStage<VolumetricLighting>();
		//mPipeline->pushStage<HDR>();
		//mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

		//mPipeline->pushStage("draw to backbuffer",[bb, quad](Renderer::Texture2D::Ptr rt)
		//{
		//	quad->setRenderTarget(bb);
		//	quad->drawTexture(rt, false);
		//});
	}

	virtual void initInput() override
	{

		mInput->listen([ this](const Input::Mouse& m, const Input::Keyboard& k) {
			auto light = mScene->createOrGetLight("main");
			auto cam = mScene->createOrGetCamera("main");

			auto node = mScene->getRoot();
			auto aabb = node->getWorldAABB();
			Vector3 vec = aabb.second - aabb.first;
			auto center = (aabb.second + aabb.first) * 0.5f;
			auto maxlen = (aabb.second - aabb.first).Length();
			cam->setNearFar(0.1, maxlen * 4);
			float com_step = std::max(std::max(vec.x, vec.y), vec.z) * 0.001f;

			
			static auto lasttime = GetTickCount();
			auto dtime = GetTickCount() - lasttime;
			lasttime = GetTickCount();
			float step = dtime * com_step;


			Vector3 cur = { (float)m.x, (float)m.y, (float)m.scrollWheelValue };
			static Vector3 last = cur;
			
			Vector3 d = cur - last;


			static Vector3 theta = {0,0,0};
			static float radius = maxlen;
			static Vector3 dir = {0,0, 1};
			//d = Vector3(d.x, d.y, d.z);
			last = cur;
			if ( d.z != 0)
			{
				radius = std::max(0.01f, radius - step - d.z * 0.02f);
			}

			if (m.leftButton)
			{
				const float degree = 3.14f * 0.5f;
				theta.x -= d.x * 0.005;
				theta.y += d.y * 0.005;
				theta.y = std::max(-degree, std::min(degree, theta.y));
				float r = cos(theta.y) * radius;
				dir = { cos(theta.x) * r, sin(theta.y) * radius, sin(theta.x) * r };
				dir.Normalize();
			}
			if (m.rightButton)
			{
				static Vector3 ldir = { 0,0,1 };
				static Vector3 o = { 0,0,maxlen };
				const float degree = 3.14f * 0.5f;

				o.x += d.x * 0.005;
				o.y -= d.y * 0.005;
				o.y = std::max(-degree, std::min(degree, o.y));
				float r = cos(o.y) * maxlen;
				ldir = { cos(o.x) * r, sin(o.y) * maxlen, sin(o.x) * r };
				ldir *= 0.25f;
				light->getNode()->setPosition(ldir);
				ldir.Normalize();
				light->setDirection(-ldir);
	
			}
			cam->lookat(dir * radius + center, center);
			return false;
		});
	}
};