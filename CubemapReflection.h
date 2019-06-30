#pragma once

#include "Pipeline.h"

class CubemapReflection : public Pipeline::Stage
{
public:
	using Pipeline::Stage::Stage;

	void init(const Vector3& pos, const std::string& cubemap);
	void render(Renderer::Texture2D::Ptr rt)  override final;

private:
	Renderer::Texture2D::Ptr mCube;
	std::shared_ptr<Pipeline> mCubePipeline;
	std::function<void(void)> mUpdate;
};