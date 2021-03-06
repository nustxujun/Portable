#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Pipeline.h"
#include "Input.h"
#include "Setting.h"
#include "ImguiOverlay.h"

class Framework: public Setting::Modifier
{
public:
	Framework(HWND win);
	virtual ~Framework();
	virtual void init();
	virtual void update();

	Setting::Ptr getSetting()const { return mPipeline->getSetting(); }

	float getFPS()const { return mFPS; }
protected:
	virtual void initPipeline();
	virtual void initScene();
	virtual void initInput();
	virtual void initUI();
	virtual void framemove() ;

	virtual void onChanged(const std::string& key, const nlohmann::json::value_type& value) {};
	void showFPS();
	void calFPS();
	void renderOverlay();
protected:
	std::unique_ptr<Pipeline> mPipeline;
	std::shared_ptr<Input> mInput;
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	ImguiOverlay::Ptr mOverlay;
	Renderer::Profile::Ptr mOverlayProfile;
	

	size_t mCachedNumFrames = 0;
	size_t mLastTimeStamp = 0;
	float mFPS = 0;
	HWND mWindow;
};