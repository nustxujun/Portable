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

	void draw(Renderer::RenderTarget::Ptr rt, std::vector<ID3D11ShaderResourceView*>& srvs, Renderer::PixelShader::Weak ps);
private:
	Renderer::Ptr mRenderer;
	Renderer::VertexShader::Weak mVS;
	Renderer::PixelShader::Weak mPS;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mVertexBuffer;
	Renderer::Buffer::Ptr mIndexBuffer;
};