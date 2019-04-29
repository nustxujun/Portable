#pragma once
#include "renderer.h"

class Test
{
	struct Vertex
	{
		float position[3];
		float texcoord[2];
	};
public:
	Test(Renderer::Ptr r);
	~Test();

	void draw(ID3D11ShaderResourceView* texture);

	Renderer::RenderTarget::Ptr mRenderTarget;

private:
	Renderer::Ptr mRenderer;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mVertexBuffer;
	Renderer::Buffer::Ptr mIndexBuffer;
	Renderer::Texture::Ptr mTexture;
	Renderer::Sampler::Ptr mSampler;
};