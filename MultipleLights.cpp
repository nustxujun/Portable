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
#include "ShadowMap.h"
#include "SkyBox.h"

#include <random>

#define ALIGN(x,y) (((x + y - 1) & ~(y - 1)) )
#define SLICE(x,y) ALIGN(x, y) / y
#define SLICE16(x) SLICE(x,16)

void MultipleLights::initPipeline()
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

	//initDRPipeline();
	//initTBDRPipeline();
	initCDRPipeline();
}

void MultipleLights::initScene()
{
	int numpoints = 100;
	int numspots = 1;
	int numdirs = 1;


	set("numpoints", { {"value", numpoints}, {"min", 1}, {"max", numpoints}, {"interval", 1}, {"type","set"} });
	set("numspots", { {"value", numspots}, {"min", 1}, {"max", numspots}, {"interval", 1}, {"type","set"} });
	set("numdirs", { {"value", numdirs}, {"min", 0}, {"max", numdirs}, {"interval", 1}, {"type","set"} });

	set("pointradiance", { {"type","set"}, {"value",1},{"min","0.1"},{"max",100},{"interval", "0.1"} });
	set("spotradiance", { {"type","set"}, {"value",1},{"min","1"},{"max",1000},{"interval", "1"} });
	set("dirradiance", { {"type","set"}, {"value",1},{"min","0.1"},{"max",100},{"interval", "0.1"} });


	set("lightRange", { {"value", 100}, {"min", 1}, {"max", 1000}, {"interval", 1}, {"type","set"} });
	set("fovy", { {"value", 0.785398185f}, {"min", "0.1"}, {"max", "2"}, {"interval", "0.01"}, {"type","set"} });

	auto root = mScene->getRoot();

	//{
	//	Parameters params;
	//	params["geom"] = "room";
	//	params["size"] = "100";
	//	auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
	//	{
	//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
	//	});
	//	model->setCastShadow(false);
	//	model->attach(root);
	//	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	//}
	//{
	//	Parameters params;
	//	params["geom"] = "cube";
	//	params["size"] = "100";
	//	auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
	//	{
	//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
	//	});
	//	model->setCastShadow(false);
	//	model->attach(root);
	//	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	//}

	//{
	//	Parameters params;
	//	params["geom"] = "plane";
	//	params["size"] = "100";
	//	//params["resolution"] = "500";
	//	auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
	//	{
	//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
	//	});
	//	model->setCastShadow(false);
	//	model->attach(root);
	//	model->getNode()->setPosition(0.0f, 0.f, 0.0f);

	//}

	//{
	//	Parameters params;
	//	params["geom"] = "sphere";
	//	params["radius"] = "10";
	//	params["resolution"] = "500";
	//	auto model = mScene->createModel(params["geom"], params, [this](const Parameters& p)
	//	{
	//		return Mesh::Ptr(new GeometryMesh(p, mRenderer));
	//	});
	//	model->setCastShadow(true);
	//	model->attach(root);
	//	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	//}

	{
		Parameters params;
		//params["file"] = "tiny.x";
		params["file"] = "media/sponza/sponza.obj";
		auto model = mScene->createModel("test", params, [this](const Parameters& p) {
			return Mesh::Ptr(new Mesh(p, mRenderer));
		});

		model->attach(mScene->getRoot());
		model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	
	}

	auto cam = mScene->createOrGetCamera("main");
	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setFOVy(0.785398185f);
	auto aabb = root->getWorldAABB();
	auto len = aabb.second - aabb.first;
	cam->setNearFar(1.0f, (len).Length());

	cam->lookat({ 0.0f, 10.0f, 0.0f }, { 1000.0f, 10.0f, 0.0f });
	//cam->getNode()->setPosition((aabb.second + aabb.first) * 0.5f);
	//cam->getNode()->setPosition(0.0f, 100.0f, 0.0f);
	//cam->setDirection({ 0,0,1 });
	//cam->lookat(aabb.second, aabb.first);
	std::uniform_real_distribution<float> rand(0.0f, 1.0f);
	std::uniform_real_distribution<float> rand2(-1.0f, 1.0f);

	std::default_random_engine gen;

	struct State
	{
		Scene::Light::Ptr light;
		Vector3 vel;
	};
	std::shared_ptr<std::vector<State>> pointlights = std::shared_ptr<std::vector<State>>(new std::vector<State>());
	std::shared_ptr<std::vector<State>> spotlights = std::shared_ptr<std::vector<State>>(new std::vector<State>());
	std::shared_ptr<std::vector<State>> dirlights = std::shared_ptr<std::vector<State>>(new std::vector<State>());
	{
		auto mainlight = mScene->createOrGetLight("main");
		//mainlight->setColor({ 0.9f, 0.7f, 0.4f });
		mainlight->setDirection({ 0,-1,0.1f });
		mainlight->setType(Scene::Light::LT_DIR);
		mainlight->setCastingShadow(true);
		dirlights->push_back({ mainlight ,{0.0f,0.f,0.f} });
	}

	for (int i = 0; i < numpoints; ++i)
	{
		std::stringstream ss;
		ss << "point_";
		ss << i;
		auto light = mScene->createOrGetLight(ss.str());
		Vector3 color = { rand(gen),rand(gen),rand(gen) };
		//color *= 1;
		light->setColor(color);
		light->setType(Scene::Light::LT_POINT);
		light->attach(mScene->getRoot());
		float step = 3.14159265358f * 2.0f / (float)numpoints;
		float t = i * step;
		float z = sin(t) * 30;
		float x = cos(t) * 30;
		light->getNode()->setPosition(rand2(gen) *len.x * 0.5f, rand2(gen) * len.y  * 0.5f, rand2(gen) * len.z  * 0.5f);
		//light->getNode()->setPosition( 0.0f, len.y  * 0.5f - 0.1f,0.0f );
		//light->getNode()->setPosition(x, 49.0f, z);
		//light->getNode()->setPosition(0.0f, 49.0f, ((i % 2) * 2.0f - 1) * 30.0f);
		//light->getNode()->setPosition((float)i * (len.x / (float)numpoints) - len.x * 0.5f, 0.0f, 120.0f);

		light->attach(root);
		Vector3 vel = { rand2(gen),rand2(gen),rand2(gen) };
		vel.Normalize();
		vel *= 1.0f;
		pointlights->push_back({ light,vel });
	}

	for (int i = 0; i < numspots; ++i)
	{
		std::stringstream ss;
		ss << "spot_";
 		ss << i;
		auto light = mScene->createOrGetLight(ss.str());
		//Vector3 color = { rand(gen),rand(gen),rand(gen) };
		Vector3 color = { 1,0,0 };
		color.Normalize();
		//color *= 1;
		light->setColor(color);
		light->setType(Scene::Light::LT_SPOT);
		light->setSpotAngle(0.3);
		light->setDirection({ 0.0f, -1.0f, 0.0f });
		light->attach(mScene->getRoot());
		light->getNode()->setPosition((aabb.first + aabb.second) * 0.5f);
		//light->attach(cam->getNode());
		light->attach(root);

		spotlights->push_back({ light,{0.0f,0.0f,0.0f} });
	}

	std::map<Scene::Light::Ptr, int> lightCastingShadow;
	int numCastingShadow = 1;

	mScene->visitLights([&numCastingShadow, &lightCastingShadow](auto light)
	{
		if (light->isCastingShadow())
			lightCastingShadow[light] = numCastingShadow++;
	});

	auto updateLights = [this, lightCastingShadow](auto lights, const std::string& buffername) {
		auto cam = mScene->createOrGetCamera("main");
		Matrix view = cam->getViewMatrix();
		D3D11_MAP map = D3D11_MAP_WRITE_DISCARD;
		D3D11_MAPPED_SUBRESOURCE subresource;
		auto context = mRenderer->getContext();
		auto buffer = mPipeline->getBuffer(buffername).lock();
		context->Map(*buffer, 0, map, 0, &subresource);
		char* data = (char*)subresource.pData;
		float range = 1000.0f;
		if (has("lightRange"))
			range = getValue<float>("lightRange");
		
		auto endi = lightCastingShadow.end();
		for (auto& l : *lights)
		{
			auto light = l.light;
			auto iter = lightCastingShadow.find(light);
			float shadowindex = 0.0f;
			if (iter != endi)
				shadowindex = (float)iter->second;

			if (light->getType() == Scene::Light::LT_POINT)
			{
				Vector3 pos = light->getNode()->getRealPosition();
				Vector4 vpos = Vector4::Transform({ pos.x, pos.y, pos.z, 1 }, view);
				vpos.w = range;
				memcpy(data, &vpos, sizeof(vpos));
				data += sizeof(vpos);
				Vector3 color = light->getColor();
				color *= getValue<float>("pointradiance");
				memcpy(data, &Vector4(color.x, color.y, color.z, shadowindex), sizeof(Vector4));
				data += sizeof(Vector4);
			}
			else if (light->getType() == Scene::Light::LT_SPOT)
			{
				Vector3 pos = light->getNode()->getRealPosition();
				Vector4 vpos = Vector4::Transform({ pos.x, pos.y, pos.z, 1 }, view);
				vpos.w = range;
				memcpy(data, &vpos, sizeof(vpos));
				data += sizeof(vpos);

				Vector3 dir = light->getDirection();
				Vector4 vdir = Vector4::Transform({ dir.x, dir.y,dir.z,0 }, view);
				vdir.w = std::cos(light->getSpotAngle());
				memcpy(data, &vdir, sizeof(vdir));
				data += sizeof(vdir);

				Vector3 color = light->getColor();
				color *= getValue<float>("spotradiance");
				memcpy(data, &Vector4(color.x, color.y, color.z, shadowindex), sizeof(Vector4));
				data += sizeof(Vector4);
			}
			else if (light->getType() == Scene::Light::LT_DIR)
			{
				Vector3 dir = light->getDirection();
				Vector4 vdir = Vector4::Transform({ dir.x, dir.y,dir.z,0 }, view);
				memcpy(data, &vdir, sizeof(vdir));
				data += sizeof(vdir);

				Vector3 color = light->getColor();
				color *= getValue<float>("dirradiance");
				memcpy(data, &Vector4(color.x, color.y, color.z, shadowindex), sizeof(Vector4));
				data += sizeof(Vector4);
			}
		}

		context->Unmap(*buffer, 0);
	};
	
	set("time", { {"value", 3.14f}, {"min", "1.57"}, {"max", "4.71"}, {"interval", "0.001"}, {"type","set"} });
	mUpdater = [=]() {
		updateLights(pointlights, "pointlights");
		updateLights(spotlights, "spotlights");
		updateLights(dirlights, "dirlights");

		//for(auto&i: *pointlights)
		//{ 
		//	auto node = i.light->getNode();
		//	auto pos = node->getPosition() + i.vel;

		//	if (pos.x > len.x || pos.x < -len.x)
		//		i.vel.x = -i.vel.x;
		//	if (pos.y > len.y || pos.y < -len.y)
		//		i.vel.y = -i.vel.y;
		//	if (pos.z > len.z || pos.z < -len.z)
		//		i.vel.z = -i.vel.z;

		//	node->setPosition(pos);
		//}

		auto mainlight = mScene->createOrGetLight("main");
		float rad = getValue<float>("time");
		float sin = std::sin(rad);
		float cos = std::cos(rad);

		mainlight->setDirection({ 0.0f,cos, sin });
	
	};


}

void MultipleLights::update()
{
	showFPS();
	//Framework::update();
	mInput->update();
	if (mUpdater)
		mUpdater();
	mPipeline->render();
	mRenderer->present();
}

void MultipleLights::initDRPipeline()
{
	Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
	auto w = mRenderer->getWidth();
	auto h = mRenderer->getHeight();


	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage("clear rt",[bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>();
	mPipeline->pushStage<DepthLinearing>();
	mPipeline->pushStage<PBR>();
	//mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage("draw to backbuffer",[bb, quad](Renderer::Texture2D::Ptr rt)
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
	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);



	int bw = SLICE16(w);
	int bh = SLICE16(h);
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

	constexpr auto maxlightspercluster = 1024;
	auto lightindices = mRenderer->createRWBuffer(maxlightspercluster * bw * bh * sizeof(UINT), sizeof(UINT), DXGI_FORMAT_R32_UINT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
	mPipeline->addShaderResource("lightindices", lightindices);
	mPipeline->addUnorderedAccess("lightindices", lightindices);

	D3D11_TEXTURE2D_DESC texdesc = {0};
	texdesc.Width = bw;
	texdesc.Height = bh;
	texdesc.MipLevels = 1;
	texdesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags = 0;
	texdesc.MiscFlags = 0;
	texdesc.ArraySize = 1;
	texdesc.SampleDesc.Quality = 0;
	texdesc.SampleDesc.Count = 1;


	auto lighttable = mRenderer->createTexture(texdesc);
	mPipeline->addShaderResource("lighttable", lighttable);
	mPipeline->addUnorderedAccess("lighttable", lighttable);


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
	mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage("draw to backbuffer",[bb, quad](Renderer::Texture2D::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});



}

void MultipleLights::initCDRPipeline()
{
	auto w = mRenderer->getWidth();
	auto h = mRenderer->getHeight();

	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);

	constexpr auto MAX_SHADOWMAPS = 8;
	std::vector<Renderer::Texture::Ptr> shadowmaps;
	for (int i = 0; i < MAX_SHADOWMAPS; ++i)
		shadowmaps.push_back(mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32_FLOAT));

	constexpr auto SLICED_Z = 16;
	constexpr auto SLICED_LEN = 16;
	int bw = SLICE(w, SLICED_LEN);
	int bh = SLICE(h, SLICED_LEN);

	constexpr auto maxlightspercluster = 1024;
	auto lightindices = mRenderer->createRWBuffer(maxlightspercluster * SLICED_LEN * SLICED_LEN * SLICED_Z * sizeof(UINT), sizeof(UINT), DXGI_FORMAT_R32_UINT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
	mPipeline->addShaderResource("lightindices", lightindices);
	mPipeline->addUnorderedAccess("lightindices", lightindices);

	D3D11_TEXTURE3D_DESC texdesc;
	texdesc.Width = SLICED_LEN;
	texdesc.Height = SLICED_LEN;
	texdesc.Depth = SLICED_Z;
	texdesc.MipLevels = 1;
	texdesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags = 0;
	texdesc.MiscFlags = 0;
	auto lighttable = mRenderer->createTexture3D(texdesc);
	mPipeline->addShaderResource("lighttable", lighttable);
	mPipeline->addUnorderedAccess("lighttable", lighttable);

	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage("clear rt", [bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>();
	mPipeline->pushStage<DepthLinearing>();

	mPipeline->pushStage<ShadowMap>(2048, 3, shadowmaps);

	mPipeline->pushStage<ClusteredLightCulling>(Vector3(SLICED_LEN, SLICED_LEN, SLICED_Z), Vector3(bw, bh,0));
	
	mPipeline->pushStage<PBR>(Vector3(bw, bh,0), shadowmaps);

	mPipeline->pushStage<AO>(10.0f);
	std::vector<std::string> files = { 
		"media/skybox/right.jpg",
		"media/skybox/left.jpg", 
		"media/skybox/top.jpg", 
		"media/skybox/bottom.jpg", 
		"media/skybox/front.jpg",
		"media/skybox/back.jpg",
	};
	//std::vector<std::string> files = { "media/uffizi_cross.dds" };
	mPipeline->pushStage<SkyBox>(files);

	mPipeline->pushStage<HDR>();

	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");
	Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
	mPipeline->pushStage("draw to backbuffer", [bb, quad](Renderer::Texture2D::Ptr rt)
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
