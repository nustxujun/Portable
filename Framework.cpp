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
Framework::Framework(HWND win)
{
	mRenderer = std::make_shared<Renderer>();
	RECT rect;
	GetClientRect(win, &rect);
	mRenderer->init(win, rect.right, rect.bottom);

	mScene = std::make_shared<Scene>();
	mPipeline = std::make_unique<Pipeline>(mRenderer, mScene);
	setSetting(mPipeline->getSetting());
	mInput = std::make_shared<Input>();
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
	mPipeline->render();
	showFPS();
	mRenderer->present();
}


void Framework::initPipeline()
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
	mPipeline->pushStage("clear rt", [bb](Renderer::Texture::Ptr rt)
	{
		bb.lock()->clear({ 1,1,1,0 });
		rt.lock()->clear({ 0,0,0,0 });
	});



	mPipeline->pushStage<GBuffer>(albedo, normal, worldpos, depth);
	//mPipeline->pushStage<PBR>(albedo, normal, depth);
	//mPipeline->pushStage<AO>(normal, depth,10.0f);
	//mPipeline->pushStage<ShadowMap>(worldpos, depth, 2048, 8);
	//mPipeline->pushStage<VolumetricLighting>();

	mPipeline->pushStage<HDR>();
	mPipeline->pushStage<PostProcessing>("hlsl/gamma_correction.hlsl");

	mPipeline->pushStage("draw to backbuffer", [bb, quad](Renderer::Texture::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt, false);
	});
}

void Framework::initScene()
{
	Parameters params;
	//params["file"] = "tiny.x";
	params["file"] = "media/sponza/sponza.obj";
	auto model = mScene->createModel("test", params, [this](const Parameters& p) {
		return Mesh::Ptr(new Mesh(p, mRenderer));
	});

	model->attach(mScene->getRoot());
	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	Matrix mat = Matrix::CreateFromYawPitchRoll(0, -3.14 / 2, 0);
	//model->getNode()->setOrientation(Quaternion::CreateFromRotationMatrix(mat));

	auto aabb = model->getWorldAABB();

	Vector3 vec = aabb.second - aabb.first;

	auto cam = mScene->createOrGetCamera("main");
	DirectX::XMFLOAT3 eye(aabb.second);
	DirectX::XMFLOAT3 at(aabb.first);
	//cam->lookat(eye, at);
	cam->lookat({ 0,0,-20 }, { 0,0,0 });
	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setNearFar(1, vec.Length() + 1);
	cam->setFOVy(0.785398185);


	auto light = mScene->createOrGetLight("main");
	light->setDirection({ 0.2,-1,0.2 });
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

