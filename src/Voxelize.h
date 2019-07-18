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
	void voxelize();
	void voxelLighting();
	void visualize();
	std::unique_ptr<std::vector<char>> readTexture3D(Renderer::Texture3D::Ptr tex);
private:
	int mSize;
	std::unordered_map<size_t, Renderer::Effect::Ptr> mEffect;
	Renderer::Rasterizer::Ptr mRasterizer;
	Renderer::ComputeShader::Weak mVoxelIllumination;
	std::function<void(void)> mInitialize;
	Renderer::Texture3D::Ptr mColor;
};

