#include "Framework.h"
#include <sstream>
#include "GBuffer.h"
#include "PBR.h"
#include "ShadowMap.h"
#include "AO.h"
#include "Prepare.h"
#include "VolumetricLighting.h"
Framework::Framework(HWND win)
{
	mRenderer = std::make_shared<Renderer>();
	RECT rect;
	GetClientRect(win, &rect);
	mRenderer->init(win, rect.right, rect.bottom);

	mScene = std::make_shared<Scene>(mRenderer);
	mPipeline = std::make_unique<Pipeline>(mRenderer, mScene);
	mInput = std::make_shared<Input>();
}

Framework::~Framework()
{
	mRenderer->uninit();
}

void Framework::init()
{
	Quad::Ptr quad = std::make_shared<Quad>(mRenderer);
	auto w = mRenderer->getWidth();
	auto h = mRenderer->getHeight();
	auto albedo = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	auto normal = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	auto worldpos = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto depth = mRenderer->createDepthStencil(w, h, DXGI_FORMAT_R32_TYPELESS,true);
	auto frame = mRenderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	
	mPipeline->setFrameBuffer(frame);
	auto bb = mRenderer->getBackbuffer();
	mPipeline->pushStage([bb](Renderer::RenderTarget::Ptr rt)
	{
		rt.lock()->clear({ 1,1,1,1 });
	});

	mPipeline->pushStage([this](Renderer::RenderTarget::Ptr rt)
	{
		auto light = mScene->createOrGetLight("main");
		auto time = GetTickCount() * 0.0002f;
		auto x = cos(time);
		auto y = sin(time);
		light->setDirection({ x, y,x });
	});

	mPipeline->pushStage<GBuffer>(albedo, normal, worldpos, depth);
	mPipeline->pushStage<PBR>(albedo, normal, depth, 0.5f, 0.5f,10.0f);
	//mPipeline->pushStage<AO>(normal, depth,10.0f);
	//mPipeline->pushStage<ShadowMap>(worldpos,depth, 2048, 8);
	mPipeline->pushStage<VolumetricLighting>();

	mPipeline->pushStage([bb,quad](Renderer::RenderTarget::Ptr rt)
	{
		quad->setRenderTarget(bb);
		quad->drawTexture(rt,false);
	});

	Parameters params;
	params["file"] = "tiny.x";
	//params["file"] = "media/sponza/sponza.obj";
	auto model = mScene->createModel("test", params);
	model->attach(mScene->getRoot());
	model->getNode()->setPosition(0.0f, 0.f, 0.0f);
	auto aabb = model->getWorldAABB();

	Vector3 vec = aabb.second - aabb.first;

	auto cam = mScene->createOrGetCamera("main");
	DirectX::XMFLOAT3 eye(aabb.second);
	DirectX::XMFLOAT3 at(aabb.first);
	cam->lookat(eye, at);

	cam->setViewport(0, 0, mRenderer->getWidth(), mRenderer->getHeight());
	cam->setNearFar(1, vec.Length());
	cam->setFOVy(0.785398185);


	auto light = mScene->createOrGetLight("main");
	light->setDirection({ 0.5,-1,0.5 });

	initInput(std::min(std::min(vec.x, vec.y), vec.z));
}


void Framework::update()
{
	mInput->update();
	mPipeline->render();
	showFPS();
	mRenderer->present();
}

void Framework::initInput(float speed)
{
	auto cam = mScene->createOrGetCamera("main");
	float com_step = speed * 0.001f;

	mInput->listen([cam, com_step](const Input::Mouse& m, const Input::Keyboard& k) {
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
	mRenderer->setRenderTarget(mRenderer->getBackbuffer());
	auto font = mRenderer->createOrGetFont(L"myfile.spritefont");

	std::stringstream ss;
	ss << mFPS;
	
	mRenderer->setViewport({
		0,0,
		(float)mRenderer->getWidth(),
		(float)mRenderer->getHeight(),
		0,1.0f
		}
	);
	font.lock()->drawText(ss.str().c_str(), Vector2(10, 10));
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
