#include "PBR.h"

PBR::PBR(
	Renderer::Ptr r, 
	Scene::Ptr s, 
	Pipeline * p, 
	Renderer::RenderTarget::Ptr a, 
	Renderer::RenderTarget::Ptr n, 
	Renderer::DepthStencil::Ptr d, 
	float roughness, 
	float metallic,
	float radiance):
	Pipeline::Stage(r,s,p),
	mAlbedo(a),
	mNormal(n),
	mDepth(d),
	mRoughness(roughness),
	mMetallic(metallic),
	mRadiance(radiance),
	mQuad(r)
{
	auto blob = r->compileFile("hlsl/pbr.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);
}

PBR::~PBR()
{
}

void PBR::render(Renderer::RenderTarget::Ptr rt)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto cam = getScene()->createOrGetCamera("main");
	auto light = getScene()->createOrGetLight("main");

	Constants constants;
	constants.radiance = mRadiance;
	constants.roughness = mRoughness;
	constants.metallic = mMetallic;

	auto dir = light->getDirection();
	dir.Normalize();
	constants.lightDir = { dir.x, dir.y,dir.z ,0};
	auto campos = cam->getNode()->getRealPosition();
	constants.cameraPos = {campos.x, campos.y, campos.z, 1.0f};
	Matrix viewproj = cam->getViewMatrix() * cam->getProjectionMatrix();
	constants.invertViewPorj = viewproj.Invert().Transpose();

	mConstants.lock()->blit(&constants, sizeof(constants));

	mQuad.setRenderTarget(rt);
	mQuad.setDefaultBlend();
	mQuad.setTextures({ mAlbedo, mNormal, mDepth });
	mQuad.setPixelShader(mPS);
	mQuad.setSamplers({ mLinear, mPoint});
	mQuad.setConstant(mConstants);
	mQuad.draw();

}
