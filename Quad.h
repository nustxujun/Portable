#pragma once
#include "renderer.h"

class Quad
{
	struct Vertex
	{
		float position[3];
		float texcoord[2];
	};
public:
	using Ptr = std::shared_ptr<Quad>;


	Quad(Renderer::Ptr r);
	~Quad();

	void draw(const std::array<float, 4>& color = {1,1,1,1});
	void drawTexture(Renderer::ShaderResource::Ptr texture, bool blend = true);

	void setRenderTarget(Renderer::RenderTarget::Ptr rt) { mRenderTarget = rt; };
	void setTextures(const std::vector<Renderer::ShaderResource::Ptr>& c) { setTexturesImpl(c); }
	template<class T>
	void setTextures(const std::vector<T>& c) { setTexturesImpl(c); }

	void setPixelShader(Renderer::PixelShader::Weak ps) { mPS = ps; };
	void setSamplers(const std::vector<Renderer::Sampler::Ptr>& ss) { mSamplers = ss; };
	void setConstant(Renderer::Buffer::Ptr c) { mConstants.clear(); mConstants.push_back(c); }
	void setConstants(const std::vector<Renderer::Buffer::Ptr>& c) { mConstants = c; }

	void setBlend(const D3D11_BLEND_DESC&desc, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);
	void setDefaultPixelShader()  { setPixelShader(mDrawTexturePS); }
	void setDefaultSampler() { setSamplers({ mDefaultSampler }); }
	void setViewport(const D3D11_VIEWPORT& vp) { mViewport = vp; }
	void setDefaultViewport() { mViewport = { 0.0f,0.0f, (float) mRenderer->getWidth(),(float) mRenderer->getHeight(), 0.0f, 1.0f }; }
	void setDefaultBlend(bool blend = true) ;
	void setBlendColorMul();

private:
	template<class T>
	void setTexturesImpl(const std::vector<T>& c)
	{
		mSRVs.clear();
		for (auto& i : c)
			mSRVs.push_back(i);
	}
private:
	Renderer::Ptr mRenderer;
	Renderer::VertexShader::Weak mVS;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mVertexBuffer;
	Renderer::Buffer::Ptr mIndexBuffer;

	Renderer::RenderTarget::Ptr mRenderTarget;
	std::vector<Renderer::ShaderResource::Ptr> mSRVs;
	Renderer::PixelShader::Weak mPS;
	std::vector<Renderer::Sampler::Ptr> mSamplers;
	std::vector<Renderer::Buffer::Ptr> mConstants;
	Renderer::PixelShader::Weak mDrawTexturePS;
	Renderer::Sampler::Ptr mDefaultSampler;
	struct BlendState
	{
		D3D11_BLEND_DESC desc;
		std::array<float, 4> factor;
		size_t mask;
	};

	BlendState mBlendState;
	D3D11_VIEWPORT mViewport;

};