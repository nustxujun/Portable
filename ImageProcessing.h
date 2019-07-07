#pragma once

#include "renderer.h"
#include "Quad.h"
#include "GeometryMesh.h"

class ImageProcessing
{
public:


	enum ResultType
	{
		RT_TEMP,
		RT_COPY,
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


	virtual Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr ptr, float scaling = 1.0f) = 0;
protected:
	Renderer::TemporaryRT::Ptr createOrGet(Renderer::Texture2D::Ptr ptr, float scaling = 1.0f);
	Renderer::TemporaryRT::Ptr createOrGet(const D3D11_TEXTURE2D_DESC & texDesc);


protected:
	Renderer::Ptr mRenderer;
	Quad mQuad;
};


class SamplingBox : public ImageProcessing
{
public:
	using Ptr = std::shared_ptr<SamplingBox>;

	using ImageProcessing::ImageProcessing;
	void init();

	virtual Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr tex, float scaling = 1.0f);
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

	virtual Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr tex, float scaling = 1.0f);
private:
	Renderer::PixelShader::Weak mPS[2];
};


class CubeMapProcessing : public ImageProcessing
{
public:
	using Ptr = std::shared_ptr<CubeMapProcessing>;
	using ImageProcessing::ImageProcessing;

	virtual void init(bool cube);


	Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr tex, float scaling = 1.0f);
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
	using Ptr = std::shared_ptr<PrefilterCubemap>;
	using CubeMapProcessing::CubeMapProcessing;

	void init(bool iscube);
	Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr tex, float scaling = 1.0f);
private:
};

class TileMaxFilter :public ImageProcessing
{
	__declspec(align(16))
		struct Constants
	{
		DirectX::SimpleMath::Vector2 texelsize;
		DirectX::SimpleMath::Vector2  offset;
		float maxradius;
		int loop;
	};
public:
	using Ptr = std::shared_ptr<TileMaxFilter>;
	using ImageProcessing::ImageProcessing;

	void init(bool normalization, float radius = 1);
	void init(const DirectX::SimpleMath::Vector2 offset, int loop);
	Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr tex, float scaling = 1.0f);
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::Buffer::Ptr mConstants;
	DirectX::SimpleMath::Vector2 mOffset;
	float mRadius;
	int mLoop;
};

class NeighborMaxFilter : public ImageProcessing
{
	__declspec(align(16))
		struct Constants
	{
		DirectX::SimpleMath::Vector2 texelsize;
	};
public:
	using Ptr = std::shared_ptr<NeighborMaxFilter>;
	using ImageProcessing::ImageProcessing;

	void init();
	Renderer::TemporaryRT::Ptr process(Renderer::Texture2D::Ptr tex, float scaling = 1.0f);
private:
	Renderer::PixelShader::Weak mPS;
	Renderer::Buffer::Ptr mConstants;
};