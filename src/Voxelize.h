#pragma once
#include "Pipeline.h"
class Voxelize :public Pipeline::Stage
{
public:
	using Pipeline::Stage::Stage;

	void init(int size);
	void render(Renderer::Texture2D::Ptr rt)  override final;
private:
	Renderer::Effect::Ptr getEffect(Material::Ptr mat);
	void voxelize(Renderer::Texture2D::Ptr rt);
private:
	std::unordered_map<size_t, Renderer::Effect::Ptr> mEffect;
	Renderer::Rasterizer::Ptr mRasterizer;
	int mSize;
	Renderer::Texture3D::Ptr mAlbedo;
};

