#include "VolumetricLighting.h"

VolumetricLighting::VolumetricLighting(Renderer::Ptr r, Scene::Ptr s, Pipeline * p):
	Pipeline::Stage(r,s,p), mQuad(r)
{
	auto blob = r->compileFile("hlsl/volumetriclight.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	auto w = r->getWidth();
	auto h = r->getHeight();
	mBlur = r->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);


	mConstants = r->createBuffer(sizeof(Vector4), D3D11_BIND_CONSTANT_BUFFER);


}

VolumetricLighting::~VolumetricLighting()
{
}

void VolumetricLighting::render(Renderer::RenderTarget::Ptr rt)
{
	renderBlur(rt);


	mQuad.setRenderTarget(rt);
	mQuad.drawTexture(mBlur,false);
}
void VolumetricLighting::renderBlur(Renderer::RenderTarget::Ptr rt)
{
	auto w = getRenderer()->getWidth();
	auto h = getRenderer()->getHeight();
	auto cam = getScene()->createOrGetCamera("main");
	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();
	Vector3 lightdir = getScene()->createOrGetLight("main")->getDirection();
	Vector4 ld = { lightdir.x, lightdir.y, lightdir.z, 0 };

	ld = Vector4::Transform(Vector4::Transform(ld, view), proj);



	Vector4 size = { (float)w,(float)h , ld.x, ld.y};
	mConstants.lock()->blit(&size, sizeof(size));


	mBlur.lock()->clear({ 1,1,1,1 });
	mQuad.setPixelShader(mPS);
	mQuad.setTextures({ rt });
	mQuad.setRenderTarget(mBlur);
	mQuad.setDefaultSampler();
	mQuad.setDefaultViewport();
	mQuad.setDefaultBlend();
	mQuad.setConstant(mConstants);

	mQuad.draw();
	getRenderer()->removeShaderResourceViews();
}
