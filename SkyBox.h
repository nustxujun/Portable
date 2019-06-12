#pragma once
#include "Pipeline.h"
#include "GeometryMesh.h"
#include "ImageProcessing.h"

class SkyBox :public Pipeline::Stage
{
public :
	using Pipeline::Stage::Stage;

	void init(const std::array<std::string,6>& texs);
	void render(Renderer::Texture2D::Ptr rt) override;
private:
	Renderer::Texture2D::Ptr mSkyTex;
	GeometryMesh::Ptr mSkyMesh;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
};
