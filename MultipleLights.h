#pragma once

#include "Framework.h"
#include <functional>

class MultipleLights : public Framework
{
public:
	using Framework::Framework;

	virtual void initPipeline();
	virtual void initScene();
	virtual void update()override;
protected:
	virtual void initDRPipeline();
	virtual void initTBDRPipeline();

	std::function<void(void)> mUpdater;
};