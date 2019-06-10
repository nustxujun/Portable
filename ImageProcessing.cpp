#include "ImageProcessing.h"

std::unordered_map<size_t, std::array< Renderer::Texture2D::Ptr, 2>> ImageProcessing::mTextures = {};

ImageProcessing::ImageProcessing(Renderer::Ptr r):mRenderer(r), mQuad(r)
{
}

Renderer::Texture2D::Ptr ImageProcessing::createOrGet(Renderer::Texture2D::Ptr ptr, SampleType st)
{
	auto desc = ptr.lock()->getDesc();
	if (st == DOWN)
	{
		desc.Width = desc.Width >> 1;
		desc.Height = desc.Height >> 1;
		if ((desc.Width == 0 && desc.Height == 0))
			return {};
	}
	else if (st == UP)
	{
		desc.Width = desc.Width << 1;
		desc.Height = desc.Height << 1;

		if (desc.Width > 8192 || desc.Height > 8192)
			return {};
	}

	desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	size_t hash = Common::hash(desc);
	auto ret = mTextures.find(hash);
	if (ret != mTextures.end())
		return (ret->second[0].lock() != ptr.lock())?ret->second[0]: ret->second[1];


	mTextures[hash][0] = mRenderer->createTexture(desc);
	mTextures[hash][1] = mRenderer->createTexture(desc);

	return mTextures[hash][0];
}


void SamplingBox::init()
{
	mPS = mRenderer->createPixelShader("hlsl/filter.hlsl", "sampleBox");
	mConstants = mRenderer->createBuffer(sizeof(DirectX::SimpleMath::Vector4), D3D11_BIND_CONSTANT_BUFFER);
}

Renderer::Texture2D::Ptr SamplingBox::process(Renderer::Texture2D::Ptr tex, SampleType st)
{
	auto ret = createOrGet(tex, st);
	if (ret.expired())
		return tex;

	auto desc = tex.lock()->getDesc();
	DirectX::SimpleMath::Vector4 constants = { 1.0f / (float)desc.Width, 1.0f / (float)desc.Height, mScale[0], mScale[1] };
	mConstants.lock()->blit(constants);
	mQuad.setConstant(mConstants);

	desc = ret.lock()->getDesc();
	mQuad.setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad.setTextures({ tex });
	mQuad.setRenderTarget(ret);
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultSampler();
	mQuad.setPixelShader(mPS);
	mQuad.draw();

	return ret;
}

void Gaussian::init()
{
	mPS[0] = mRenderer->createPixelShader("hlsl/gaussianblur.hlsl", "main");
	D3D10_SHADER_MACRO macros[] = { { "HORIZONTAL", "1"}, {NULL,NULL} };
	mPS[1] = mRenderer->createPixelShader("hlsl/gaussianblur.hlsl", "main", macros);

}

Renderer::Texture2D::Ptr Gaussian::process(Renderer::Texture2D::Ptr tex, SampleType st)
{
	auto ret = createOrGet(tex, st);
	if (ret.expired())
		return tex;
	auto desc = ret.lock()->getDesc();
	mQuad.setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultSampler();

	mQuad.setTextures({ tex });
	mQuad.setRenderTarget(ret);
	mQuad.setPixelShader(mPS[0]);
	mQuad.draw();

	mQuad.setTextures({ ret });
	ret = createOrGet(ret, st);
	mQuad.setRenderTarget(ret);
	mQuad.setPixelShader(mPS[1]);
	mQuad.draw();

	return ret;
}
