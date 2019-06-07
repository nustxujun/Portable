#pragma once
#include "Pipeline.h"
#include "GeometryMesh.h"

class SkyBox :public Pipeline::Stage
{
public :
	using Pipeline::Stage::Stage;

	void init(const std::vector<std::string>& texs);
	void render(Renderer::Texture2D::Ptr rt) override;
private:
	Renderer::Texture::Ptr mSkyTex;
	GeometryMesh::Ptr mSkyMesh;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
};
