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
	Quad(Renderer::Ptr r);
	~Quad();

	void draw(const std::array<float, 4>& color = {0,0,0,0});

	void setRenderTarget(Renderer::RenderTarget::Ptr rt) { mRenderTarget = rt; };
	void setTextures(const std::vector<Renderer::RenderTarget::Ptr>& rts);
	void setTextures(const std::vector<Renderer::Texture::Ptr>& ts);
	void setPixelShader(Renderer::PixelShader::Weak ps) { mPS = ps; };
	void setSamplers(const std::vector<Renderer::Sampler::Ptr>& ss) { mSamplers = ss; };
	void setConstant(Renderer::Buffer::Ptr c) { mConstant = c; }
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
};