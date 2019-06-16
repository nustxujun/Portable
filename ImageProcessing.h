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

	enum ResultType
	{
		RT_TEMP,
		RT_COPY,
	};


	using Ptr = std::shared_ptr<ImageProcessing>;
	ImageProcessing(Renderer::Ptr r, ResultType type);

	template<class T, class ... Args>
	static std::shared_ptr<T> create(Renderer::Ptr r, ResultType type, const Args& ... args)
	{
		std::shared_ptr<T> p = std::shared_ptr<T>(new T(r, type));
		p->init(args...);
		return p;
	}


	virtual Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr ptr, SampleType st = DEFAULT) = 0;
protected:
	Renderer::Texture2D::Ptr createOrGet(Renderer::Texture2D::Ptr ptr, SampleType st);
	Renderer::Texture2D::Ptr createOrGet(const D3D11_TEXTURE2D_DESC & texDesc);


protected:
	Renderer::Ptr mRenderer;
	Quad mQuad;
	ResultType mType = RT_TEMP;
private:
	static std::unordered_map<size_t,std::vector< Renderer::Texture2D::Ptr>> mTextures;
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

	virtual void init(bool cube);


	Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr tex, SampleType st = DEFAULT);
protected:
	Renderer::Effect::Ptr mEffect;
	GeometryMesh::Ptr mCube;
	Renderer::Layout::Ptr mLayout;
	bool mIsCubemap;

};

class IrradianceCubemap : public CubeMapProcessing
{
public:
	using Ptr = std::shared_ptr<IrradianceCubemap>;
	using CubeMapProcessing::CubeMapProcessing;

	void init(bool);
};

class PrefilterCubemap : public CubeMapProcessing
{
public:
	using Ptr = std::shared_ptr<IrradianceCubemap>;
	using CubeMapProcessing::CubeMapProcessing;

	void init(bool iscube);
	Renderer::Texture2D::Ptr process(Renderer::Texture2D::Ptr tex, SampleType st = DEFAULT);
private:
};