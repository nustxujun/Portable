#pragma once

#include "Pipeline.h"

class EnvironmentMapping : public Pipeline::Stage
{
	__declspec(align(16))
		struct Constants
	{
		Vector3 envCubeSize;
		float envCubeScale;


	};
public:
	using Pipeline::Stage::Stage;

	void init(const Vector3& pos, const Vector3& size,const std::string& cubemap, int resolution = 512);
	void render(Renderer::Texture2D::Ptr rt)  override final;

private:
	Renderer::Texture2D::Ptr mCube;
	std::shared_ptr<Pipeline> mCubePipeline;
	std::function<void(void)> mUpdate;
	Renderer::Texture2D::Ptr mPS;
};