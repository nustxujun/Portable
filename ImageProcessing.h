#pragma once

#include "renderer.h"
#include "Quad.h"
#include "GeometryMesh.h"

class ImageProcessing
{
public:
	enum SampleType
	{
		DEFAULT,
		DOWN,
		UP
	};


	using Ptr = std::shared_ptr<ImageProcessing>;
	ImageProcessing(Renderer::Ptr r);

	template<class T, class ... Args>
	static std::shared_ptr<T> create(Renderer::Ptr r, const Args& ... args)
	{
		std::shared_ptr<T> p = std::shared_ptr<T>(new T(r));
		p->init(args...);
		return p;
	}


	virtual Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr ptr, SampleType st = DEFAULT) = 0;
protected:
	Renderer::Texture2D::Ptr createOrGet(Renderer::Texture2D::Ptr ptr, SampleType st);

protected:
	Renderer::Ptr mRenderer;
	Quad mQuad;

private:
	static std::unordered_map<size_t,std::array< Renderer::Texture2D::Ptr,2>> mTextures;
};


class SamplingBox : public ImageProcessing
{
public:
	using Ptr = std::shared_ptr<SamplingBox>;

	using ImageProcessing::ImageProcessing;
	void init();

	virtual Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr tex, SampleType st = DEFAULT);
	void setScale(float ws, float hs) { mScale = { ws, hs }; };
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::Buffer::Ptr mConstants;
	std::array<float, 2> mScale = { 0.5f, 0.5f };
};


class Gaussian:public ImageProcessing
{
public:
	using Ptr = std::shared_ptr<Gaussian>;

	using ImageProcessing::ImageProcessing;
	void init();

	virtual Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr tex, SampleType st = DEFAULT);
private:
	Renderer::PixelShader::Weak mPS[2];
};


class CubeMapProcessing : public ImageProcessing
{
public:
	using Ptr = std::shared_ptr<CubeMapProcessing>;
	using ImageProcessing::ImageProcessing;

	virtual void init();


	Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr tex, SampleType st = DEFAULT);
protected:
	Renderer::Effect::Ptr mEffect;
	GeometryMesh::Ptr mCube;
	Renderer::Layout::Ptr mLayout;
};

class IrradianceCubemap : public CubeMapProcessing
{
public:
	using Ptr = std::shared_ptr<IrradianceCubemap>;
	using CubeMapProcessing::CubeMapProcessing;

	void init();
};

class PrefilterCubemap : public CubeMapProcessing
{
public:
	using Ptr = std::shared_ptr<IrradianceCubemap>;
	using CubeMapProcessing::CubeMapProcessing;

	void init();
};