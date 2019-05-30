#pragma once

#include "Pipeline.h"

class ClusteredLightCulling: public Pipeline::Stage
{
public:
	using Pipeline::Stage::Stage;

	void init() override;
	virtual void render(Renderer::Texture::Ptr rt)override;

private:
	Renderer::ComputeShader::Weak mCS;
};