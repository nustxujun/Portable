#pragma once

#include "Pipeline.h"
#include "ImageProcessing.h"
class MotionBlur : public Pipeline::Stage
{
	__declspec(align(16))
		struct Constants
	{
		Matrix invertViewProj;
		Matrix lastView;
		Matrix proj;
	};


public:
	using Pipeline::Stage::Stage;

	void init();
	void render(Renderer::Texture2D::Ptr rt)  override final;
private:
	void renderMotionVector();
	void reconstruct(Renderer::Texture2D::Ptr rt);
private:
	Matrix mLastViewMatrix = Matrix::Identity;
	Renderer::PixelShader::Weak mPS;
	Renderer::PixelShader::Weak mVelocitySetup;
	Renderer::PixelShader::Weak mReconstruct;
	Renderer::Effect::Ptr mMotionVectorEffect;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Texture2D::Ptr mFrame;
	Renderer::Texture2D::Ptr mMotionVector;
	std::unordered_map<size_t, Matrix> mLastWorld;

	ImageProcessing::Ptr mTileMaxNormlaize;
	ImageProcessing::Ptr mTileMax2;
	ImageProcessing::Ptr mTileMaxVariable;
	ImageProcessing::Ptr mNeighborMax;

};