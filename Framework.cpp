#include "Framework.h"
#include <sstream>
#include "GBuffer.h"
#include "PBR.h"
#include "ShadowMap.h"
#include "AO.h"
#include "VolumetricLighting.h"
#include "HDR.h"
#include "GeometryMesh.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "SSR.h"

Framework::Framework(HWND win):mWindow(win)
{
	mRenderer = std::make_shared<Renderer>();
	RECT rect;
	GetClientRect(win, &rect);
	mRenderer->init(win, rect.right, rect.bottom);

	mScene = std::make_shared<Scene>();
	mPipeline = std::make_unique<Pipeline>(mRenderer, mScene);
	setSetting(mPipeline->getSetting());
	mInput = std::make_shared<Input>();
	mOverlayProfile = mRenderer->createProfile();
}

Framework::~Framework()
{
	mRenderer->uninit();
}

void Framework::init()
{
	initScene();
	initInput();

	initPipeline();
}

void Framework::update()
{
	mInput->update();
	framemove();
	mPipeline->render();
	showFPS();

	renderOverlay();
	mRenderer->present();


	std::stringstream ss;
	ss.precision(4);
	ss << getFPS();

	auto cam = mScene->createOrGetCamera("main");
	auto pos = cam->getNode()->getRealPosition();
	SetWindowTextA(mWindow, Common::format(ss.str(), "(", pos.x, ",", pos.y, ",",pos.z,")").c_str());
}

//void Framework::initOverlay(int width, int height)
//{

//}


void Framework::setOverlay(Overlay::Ptr overlay)
{
	mOverlay = overlay;
	overlay->setInput(mInput);
	overlay->setRenderer(mRenderer);
}

void Framework::initPipeline()
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
	mPipeline->pushStage("clear rt", [bb, this](Renderer::Texture2D::Ptr rt)
	{
		mRenderer->clearRenderTarget(rt, { 0,0,0,0 });
	});

	mPipeline->pushStage<GBuffer>();
	mPipeline->pushStage<ShadowMap>(2048, 3, shadowmaps);
	mPipeline->pushStage<PBR>(Vector3(), shadowmaps);
	mPipeline->pushStage<SSR>();
	//mPipeline->pushStage<AO>(3.0f);
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

void Framework::initScene()
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
		params["size"] = "100";
		auto model = mScene->createModel("plane", params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});
		model->setCastShadow(false);
		model->attach(root);
		Material::Ptr mat = Material::create();
		mat->roughness = 0;
		mat->metallic = 0;
		for (int i = 0; i < textures.size(); ++i)
			if (!textures[i].empty())
				mat->setTexture(i, mRenderer->createTexture(textures[i]));
		model->setMaterial(mat);
	}
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
		auto model = mScene->createModel("sphere", params, [this](const Parameters& p)
		{
			return Mesh::Ptr(new GeometryMesh(p, mRenderer));
		});
		model->getNode()->setPosition({ 0, 1, 0 });
		model->setCastShadow(false);
		model->attach(root);
		Material::Ptr mat = Material::create();
		//mat->roughness = 0.2;

		for (int i = 0; i < textures.size(); ++i)
			if (!textures[i].empty())
				mat->setTexture(i, mRenderer->createTexture(textures[i]));
		model->setMaterial(mat);
	}


	//{
	//	Parameters params;
	//	//params["file"] = "tiny.x";
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
	cam->lookat({ 10,10,0 }, { 10,0,0 });
	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setNearFar(1, vec.Length() + 1);
	cam->setFOVy(0.785398185);


	auto light = mScene->createOrGetLight("main");
	light->setDirection({0,-1,0 });
	light->setCastingShadow(true);
}

void Framework::framemove()
{
	auto lightbuffer = mPipeline->getBuffer("dirlights");
	Vector4 lightinfo[2];
	auto l = mScene->createOrGetLight("main");
	float rad = getValue<float>("time");
	l->setDirection({ 0.1f, cos(rad), sin(rad) });
	auto dir = l->getDirection();
	lightinfo[0] = { dir.x, dir.y,dir.z , 0 };
	auto color = l->getColor() * getValue<float>("dirradiance");
	lightinfo[1] = { color.x, color.y, color.z , 1 };
	lightbuffer.lock()->blit(lightinfo, sizeof(lightinfo));
}


void Framework::initInput()
{
	auto aabb = mScene->getRoot()->getWorldAABB();
	Vector3 vec = aabb.second - aabb.first;

	auto cam = mScene->createOrGetCamera("main");
	float com_step = std::max(std::max(vec.x, vec.y), vec.z) * 0.001f;

	mInput->listen([=](const Input::Mouse& m, const Input::Keyboard& k) {
		static auto lasttime = GetTickCount();
		auto dtime = GetTickCount() - lasttime;
		lasttime = GetTickCount();
		float step = dtime * com_step;

		auto node = cam->getNode();
		auto pos = node->getPosition();
		auto dir = cam->getDirection();
		Vector3 up(0, 1, 0);
		Vector3 r = up.Cross(dir);
		r.Normalize();

		Vector2 cur = Vector2(m.x, m.y);
		static Vector2 last = cur;
		Vector2 d = cur - last;


		d = Vector2(d.x, d.y);
		last = cur;
		if (k.W)
		{
			pos = dir * step + pos;
		}
		else if (k.S)
		{
			pos = dir * -step + pos;
		}
		else if (k.A)
		{
			pos = r * -step + pos;
		}
		else if (k.D)
		{
			pos = r * step + pos;
		}

		if (m.leftButton)
		{
			float pi = 3.14159265358;
			Quaternion rot = Quaternion::CreateFromYawPitchRoll(d.x * 0.005, d.y  * 0.005, 0);

			rot *= node->getOrientation();
			dir = Vector3::Transform(Vector3(0, 0, 1), rot);
			cam->setDirection(dir);
		}

		node->setPosition(pos);
		return false;
	});


}

void Framework::showFPS()
{
	calFPS();
	std::stringstream ss;
	ss << mFPS;
	ss << "(";
	ss.precision(4);
	ss << (float)1000.0f / (float)mFPS << ")";
	set("msg", { {"msg",ss.str()} });

	//mRenderer->setRenderTarget(mRenderer->getBackbuffer());
	//auto font = mRenderer->createOrGetFont(L"myfile.spritefont");

	//std::stringstream ss;
	//ss << mFPS;
	//
	//mRenderer->setViewport({
	//	0,0,
	//	(float)mRenderer->getWidth(),
	//	(float)mRenderer->getHeight(),
	//	0,1.0f
	//	}
	//);
	//font.lock()->drawText(ss.str().c_str(), Vector2(10, 10));
}

void Framework::calFPS()
{
	mCachedNumFrames++;
	size_t cur = GetTickCount();
	auto dur = cur - mLastTimeStamp;
	if (dur < 1000)
		return;
	mFPS = mCachedNumFrames * 1000 / dur;
	mLastTimeStamp = cur;
	mCachedNumFrames = 0;
}

void Framework::renderOverlay()
{
	if (!mOverlay)
		return;

	PROFILE(mOverlayProfile);
	mOverlay->update();
	set("overlay", { {"cost",mOverlayProfile.lock()->toString()}, {"type", "stage"} });
}
