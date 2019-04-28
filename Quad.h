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

	void draw(ID3D11ShaderResourceView* texture);
private:
	Renderer::Ptr mRenderer;
	Renderer::Effect::Ptr mEffect;
	Renderer::Layout::Ptr mLayout;
	Renderer::Buffer::Ptr mVertexBuffer;
	Renderer::Buffer::Ptr mIndexBuffer;
};