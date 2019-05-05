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
	template<class T>
	void drawTexture(T texture)
	{
		setTextures({ texture });
		setDefaultPixelShader();
		setDefaultSampler();
		D3D11_BLEND_DESC desc = { 0 };
		//desc.RenderTarget[0] = {
		//	TRUE,
		//	D3D11_BLEND_SRC_ALPHA,
		//	D3D11_BLEND_INV_SRC_ALPHA,
		//	D3D11_BLEND_OP_ADD,
		//	D3D11_BLEND_ONE,
		//	D3D11_BLEND_ZERO,
		//	D3D11_BLEND_OP_ADD,
		//	D3D11_COLOR_WRITE_ENABLE_ALL,
		//};
		desc.RenderTarget[0] = {
			TRUE,
			D3D11_BLEND_SRC_ALPHA,
			D3D11_BLEND_ZERO,
			D3D11_BLEND_OP_ADD,
			D3D11_BLEND_ONE,
			D3D11_BLEND_ZERO,
			D3D11_BLEND_OP_ADD,
			D3D11_COLOR_WRITE_ENABLE_ALL
		};



		setBlend(desc);
		draw();
	}


	void setRenderTarget(Renderer::RenderTarget::Ptr rt) { mRenderTarget = rt; };
	void setTextures(const std::vector<Renderer::RenderTarget::Ptr>& rts);
	void setTextures(const std::vector<Renderer::Texture::Ptr>& ts);
	void setPixelShader(Renderer::PixelShader::Weak ps) { mPS = ps; };
	void setSamplers(const std::vector<Renderer::Sampler::Ptr>& ss) { mSamplers = ss; };
	void setConstant(Renderer::Buffer::Ptr c) { mConstant = c; }
	void setBlend(const D3D11_BLEND_DESC&desc, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);
	void setDefaultPixelShader()  { setPixelShader(mDrawTexturePS); }
	void setDefaultSampler() { setSamplers({ mDefaultSampler }); }
private:
	Renderer::Ptr mRenderer;
	Renderer::VertexShader::Weak mVS;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mVertexBuffer;
	Renderer::Buffer::Ptr mIndexBuffer;

	Renderer::RenderTarget::Ptr mRenderTarget;
	std::vector<ID3D11ShaderResourceView*> mSRVs;
	Renderer::PixelShader::Weak mPS;
	std::vector<Renderer::Sampler::Ptr> mSamplers;
	Renderer::Buffer::Ptr mConstant;
	Renderer::PixelShader::Weak mDrawTexturePS;
	Renderer::Sampler::Ptr mDefaultSampler;
	struct BlendState
	{
		D3D11_BLEND_DESC desc;
		std::array<float, 4> factor;
		size_t mask;
	};

	BlendState mBlendState;

};