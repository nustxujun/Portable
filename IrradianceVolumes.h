#pragma once

#include "Pipeline.h"
class IrradianceVolumes:public Pipeline::Stage
{
	static const size_t SH_DEGREE;// nums of coefs == ( degree + 1 )^2
	static const size_t NUM_COEFS;
	static const size_t COEFS_ARRAY_SIZE;
	static const size_t RESOLUTION;
	ALIGN16 struct Constants
	{
		Matrix invertViewProj;
		Vector3 origin;
		float intensity;
		Vector3 range;
	};
public:
	using Pipeline::Stage::Stage;
	void render(Renderer::Texture2D::Ptr rt)  override final;
	void init(const Vector3& size, const std::string& sky);

private:
	void initBakedPipeline(const std::string& sky);
	void calCoefs(size_t cubesize, Renderer::Texture2D::Ptr frame);
	void readTextureCube(Renderer::Texture2D::Ptr cube, std::array<std::vector<Vector4>, 6>& contents);
private:
	size_t mDegree;
	Vector3 mSize;
	std::vector<Renderer::Texture3D::Ptr> mCoefficients;
	std::unique_ptr<Pipeline> mBakedPipeline;
	std::function<void(void)> mCalSH;
	Renderer::PixelShader::Weak mPS;
	Renderer::Buffer::Ptr mConstants;
};