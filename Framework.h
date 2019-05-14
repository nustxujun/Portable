#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Pipeline.h"
#include "Input.h"

class Framework
{
public:
	Framework(HWND win);
	virtual ~Framework();
	virtual void init();
	virtual void update();
protected:
	virtual void initInput(float speed);
private:
	void showFPS();
	void calFPS();
protected:
	std::unique_ptr<Pipeline> mPipeline;
	std::shared_ptr<Input> mInput;
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	

	size_t mCachedNumFrames = 0;
	size_t mLastTimeStamp = 0;
	size_t mFPS = 0;
};