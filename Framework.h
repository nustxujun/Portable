#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Pipeline.h"
#include "Input.h"
#include "Setting.h"

class Framework: public Setting::Modifier
{
public:
	Framework(HWND win);
	virtual ~Framework();
	virtual void init();
	virtual void update();

	Setting::Ptr getSetting()const { return mPipeline->getSetting(); }
protected:
	virtual void initPipeline();
	virtual void initScene();
	virtual void initInput();

	virtual void onChanged(const std::string& key, const nlohmann::json::value_type& value) {};
	void showFPS();
	void calFPS();
protected:
	std::unique_ptr<Pipeline> mPipeline;
	std::shared_ptr<Input> mInput;
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;

	

	size_t mCachedNumFrames = 0;
	size_t mLastTimeStamp = 0;
	float mFPS = 0;
};