#pragma once

#include "Pipeline.h"
#include "ImageProcessing.h"
class VolumetricLighting:public Pipeline::Stage
{
	ALIGN16 struct Constants
	{
		Matrix invertViewProj;
		Matrix view;
		Matrix lightView;
		Matrix lightProjs[8];
		Vector4 cascadeParams[8];
		int numcascades;

		Vector3 campos;
		int numsamples;
		Vector3 lightcolor;
		float jitter;
		Vector3 lightdir;
		float maxlength;
		float density;
		Vector2 noiseSize;
		Vector2 screenSize;
	};
public:
	VolumetricLighting(Renderer::Ptr r, Scene::Ptr s,Quad::Ptr q, Setting::Ptr set, Pipeline* p);
	~VolumetricLighting();

	void render(Renderer::Texture2D::Ptr rt)  override final;
	void init();
private:
	void renderBlur(Renderer::Texture2D::Ptr rt);
private:
	Renderer::PixelShader::Weak mPS[3];
	Renderer::PixelShader::Weak mColorFilter;

	Renderer::Texture2D::Ptr mBlur;
	Quad mQuad;
	Renderer::Buffer::Ptr mConstants;
	Renderer::Sampler::Ptr mLinearClamp;
	Renderer::Sampler::Ptr mSampleCmp ;
	ImageProcessing::Ptr mDownsample;
	ImageProcessing::Ptr mGauss;
	Renderer::BlendState::Weak mBlend;
	Renderer::Texture2D::Ptr mNoise;
};