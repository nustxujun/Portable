#pragma once
#include "renderer.h"
#include "Quad.h"
class Sampling
{
public:
	using Ptr = std::shared_ptr<Sampling>;
	Sampling(Renderer::Ptr r);

	virtual Renderer::Texture2D::Ptr sample(Renderer::Texture2D::Ptr tex) = 0;

	Renderer::Texture2D::Ptr createOrGetTexture(const D3D11_TEXTURE2D_DESC& desc);
protected:
	Renderer::Ptr mRenderer;
	Quad::Ptr mQuad;

	static std::unordered_map<size_t, Renderer::Texture2D::Ptr> mTextures;
};

class DownSamplingBox : public Sampling
{
public:
	using Ptr = std::shared_ptr<DownSamplingBox>;
	static Ptr create(Renderer::Ptr r)
	{
		auto smp = Ptr(new DownSamplingBox(r));
		smp->init();
		return smp;
	}

	using Sampling::Sampling;
	void init();

	virtual Renderer::Texture2D::Ptr sample(Renderer::Texture2D::Ptr tex);

private:
	Renderer::PixelShader::Weak mPS;
};

class UpSamplingBox : public Sampling
{
public:
	using Ptr = std::shared_ptr<UpSamplingBox>;
	using Sampling::Sampling;
	void init();

	static Ptr create(Renderer::Ptr r)
	{
		auto smp = Ptr(new UpSamplingBox(r));
		smp->init();
		return smp;
	}

	virtual Renderer::Texture2D::Ptr sample(Renderer::Texture2D::Ptr tex);

private:
	Renderer::PixelShader::Weak mPS;
};