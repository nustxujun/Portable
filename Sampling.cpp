#include "Sampling.h"

std::unordered_map<size_t, Renderer::Texture2D::Ptr> Sampling::mTextures = {};

Sampling::Sampling(Renderer::Ptr r) :mRenderer(r)
{
	mQuad = Quad::Ptr(new Quad(r));	
}

Renderer::Texture2D::Ptr Sampling::createOrGetTexture(const D3D11_TEXTURE2D_DESC & desc)
{
	size_t hash = Common::hash(desc);
	auto ret = mTextures.find(hash);
	if (ret != mTextures.end())
		return ret->second;

	mTextures[hash] = mRenderer->createTexture(desc);
	return mTextures[hash];
}


void DownSamplingBox::init()
{
	mPS = mRenderer->createPixelShader("hlsl/sampling.hlsl", "downSampleBox");
}

Renderer::Texture2D::Ptr DownSamplingBox::sample(Renderer::Texture2D::Ptr tex)
{
	auto desc = tex.lock()->getDesc();
	desc.Width = desc.Width >> 1;
	desc.Height = desc.Height >> 1;

	if (desc.Width == 0 && desc.Height == 0)
		return tex;

	desc.Width = std::max(desc.Width, 1U);
	desc.Height = std::max(desc.Height, 1U);

	auto ret = createOrGetTexture(desc);

	mQuad->setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad->setTextures({ tex });
	mQuad->setRenderTarget(ret);
	mQuad->setDefaultBlend(false);
	mQuad->setDefaultSampler();
	mQuad->setPixelShader(mPS);
	mQuad->draw();

	return ret;
}

void UpSamplingBox::init()
{
	mPS = mRenderer->createPixelShader("hlsl/sampling.hlsl", "downSampleBox");
}

Renderer::Texture2D::Ptr UpSamplingBox::sample(Renderer::Texture2D::Ptr tex)
{
	auto desc = tex.lock()->getDesc();
	desc.Width = desc.Width << 1;
	desc.Height = desc.Height << 1;

	if (desc.Width > 8192 || desc.Height > 8192)
		return tex;

	auto ret = createOrGetTexture(desc);

	mQuad->setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad->setTextures({ tex });
	mQuad->setRenderTarget(ret);
	mQuad->setDefaultBlend(false);
	mQuad->setDefaultSampler();
	mQuad->setPixelShader(mPS);
	mQuad->draw();

	return ret;
}
