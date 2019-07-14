#include "DepthLinearing.h"

DepthLinearing::DepthLinearing(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr set, Pipeline * p):
	Pipeline::Stage(r,s,q,set,p)
{
	mName = "linear depth";
	mPS = r->createPixelShader("hlsl/depthlinearing.hlsl", "main");

	mConstants = r->createBuffer(sizeof(Matrix), D3D11_BIND_CONSTANT_BUFFER);

	mDepth = getShaderResource("depth");
	mDepthLinear = getRenderTarget("depthlinear");
}

void DepthLinearing::render(Renderer::Texture2D::Ptr rt)
{
	auto cam = getCamera();
	Matrix proj = cam->getViewMatrix().Invert().Transpose();
	mConstants.lock()->blit(&proj, sizeof(proj));

	auto quad = getQuad();
	quad->setConstant(mConstants);
	quad->setTextures({ mDepth });
	quad->setRenderTarget(mDepthLinear);
	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setDefaultBlend(false);
	quad->setPixelShader(mPS);
	quad->draw();
}
