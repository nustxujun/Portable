#include "PreZ.h"

void PreZ::init()
{
	mName = "preZ pass";
	mVS = getRenderer()->createVertexShader("hlsl/simple_vs.hlsl");
	mConstants = getRenderer()->createConstantBuffer(sizeof(Constants));

	D3D11_INPUT_ELEMENT_DESC depthlayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = getRenderer()->createLayout(depthlayout, ARRAYSIZE(depthlayout));
}

void PreZ::render(Renderer::Texture2D::Ptr rt)
{
	auto depth = getDepthStencil("depth");
	auto renderer = getRenderer();
	renderer->clearDepth(depth, 1.0f);

	auto scene = getScene();
	auto cam = scene->createOrGetCamera("main");
	Constants c;
	c.view = cam->getViewMatrix().Transpose();
	c.proj = cam->getProjectionMatrix().Transpose();

	renderer->setRenderTarget({}, depth);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->setDefaultDepthStencilState();
	//renderer->setSampler(mPoint);
	renderer->setVertexShader(mVS);
	renderer->setPixelShader({});

	renderer->setLayout(mLayout.lock()->bind(mVS));
	renderer->setDefaultRasterizer();

	renderer->setViewport(cam->getViewport());

	scene->visitRenderables([&c,renderer, this](const Renderable& r)
	{
		c.world = r.tranformation.Transpose();
		mConstants.lock()->blit(c);
		renderer->setVSConstantBuffers({ mConstants });

		renderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
		renderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
		renderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
	});
}
