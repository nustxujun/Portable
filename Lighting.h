#pragma once
#include "renderer.h"

class Lighting
{
public:
	Lighting(Renderer::Ptr renderer);
	~Lighting();
	
	Renderer::RenderTarget::Ptr getRenderTarget() { return mLightingMap; };
	Renderer::Effect::Ptr getEffect() { return mEffect; }

	void prepare();
private:
	Renderer::Effect::Ptr mEffect;
	Renderer::Ptr mRenderer;
	Renderer::RenderTarget::Ptr mLightingMap;
};
