#include "VolumetricLighting.h"

VolumetricLighting::VolumetricLighting(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline * p):
	Pipeline::Stage(r,s,q,st,p), mQuad(r)
{
	auto blob = r->compileFile("hlsl/volumetriclight.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	blob = r->compileFile("hlsl/volumetriclight_filtercolor.hlsl", "main", "ps_5_0");
	mColorFilter = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	auto vp = getCamera()->getViewport();
	mBlur = r->createRenderTarget(vp.Width, vp.Height, DXGI_FORMAT_R8G8B8A8_UNORM);


	mConstants = r->createBuffer(sizeof(Vector4), D3D11_BIND_CONSTANT_BUFFER);
	mLinearClamp = r->createSampler("linear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

}

VolumetricLighting::~VolumetricLighting()
{
}

void VolumetricLighting::render(Renderer::Texture2D::Ptr rt) 
{
	renderBlur(rt);


	//mQuad.setRenderTarget(rt);
	//mQuad.drawTexture(mBlur,false);
}
void VolumetricLighting::renderBlur(Renderer::Texture2D::Ptr rt)
{
	auto w = getRenderer()->getWidth();
	auto h = getRenderer()->getHeight();
	auto cam = getCamera();
	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();
	Vector3 lightdir = -getScene()->createOrGetLight("main")->getDirection();
	Vector4 ld = { lightdir.x, lightdir.y, lightdir.z, 0 };

	ld = Vector4::Transform(Vector4::Transform(ld, view), proj);



	Vector4 size = { (float)w,(float)h , ld.x, ld.y};
	mConstants.lock()->blit(&size, sizeof(size));


	//mBlur->getRenderTarget().lock()->clear({ 1,1,1,1 });
	getRenderer()->clearRenderTarget(mBlur, { 1,1,1,1 });

	mQuad.setSamplers({ mLinearClamp });
	mQuad.setDefaultViewport();
	mQuad.setDefaultBlend(false);
	mQuad.setConstant(mConstants);

	mQuad.setPixelShader(mColorFilter);
	mQuad.setTextures({ rt });
	mQuad.setRenderTarget(mBlur);
	mQuad.draw();

	D3D11_BLEND_DESC desc = { 0 };

	desc.RenderTarget[0] = {
		TRUE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};

	mQuad.setBlend(desc);

	mQuad.setPixelShader(mPS);
	mQuad.setTextures({ mBlur });
	mQuad.setRenderTarget(rt);

	mQuad.draw();

	getRenderer()->removeShaderResourceViews();
}
