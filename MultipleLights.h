#pragma once

#include "Framework.h"

class MultipleLights : public Framework
{
public:
	using Framework::Framework;

	virtual void initPipeline();
	virtual void initScene();
};