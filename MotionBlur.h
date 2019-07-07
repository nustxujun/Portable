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

	
	ALIGN16 struct RecotConstants
	{
		Matrix invertProj;

		Vector2 _CameraMotionVectorsTexture_TextureSize;
		Vector2 _VelocityScale;

		Vector2 _VelocityTex_TexelSize;
		Vector2 _NeighborMaxTex_TexelSize;

		Vector2 _MainTex_TexelSize;
		float _MaxBlurRadius;
		float _RcpMaxBlurRadius;
		int _LoopCount;
	};
public:
	using Pipeline::Stage::Stage;

	void init(bool cameraonly = true);
	void render(Renderer::Texture2D::Ptr rt)  override final;
private:
	void renderMotionVector();
	void reconstruct(Renderer::Texture2D::Ptr rt);
	void renderCameraMB(Renderer::Texture2D::Ptr rt);
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
	Renderer::Buffer::Ptr mRecotConstants;
	size_t mTileSize;
	float mMaxBlurRadius;
};