#include "PBR.h"

PBR::PBR(
	Renderer::Ptr r, 
	Scene::Ptr s, 
	Pipeline * p, 
	Renderer::RenderTarget::Ptr a, 
	Renderer::RenderTarget::Ptr n, 
	Renderer::DepthStencil::Ptr d, 
	float roughness, 
	float metallic):
	Pipeline::Stage(r,s,p),
	mAlbedo(a),
	mNormal(n),
	mDepth(d),
	mRoughness(roughness),
	mMetallic(metallic),
	mQuad(r)
{
	const std::array<const char*, 3> definitions = { "POINT", "DIR", "SPOT" };
	for (int i = 0; i < definitions.size(); ++i)
	{
		D3D10_SHADER_MACRO macros[] = { {definitions[i],""}, NULL, NULL };
		auto blob = r->compileFile("hlsl/pbr.hlsl", "main", "ps_5_0", macros);
		mPSs[i] = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}


	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER,NULL, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

PBR::~PBR()
{
}

void PBR::render(Renderer::RenderTarget::Ptr rt)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto cam = getScene()->createOrGetCamera("main");



	getScene()->visitLights([this,cam,rt](Scene::Light::Ptr light) {
		Constants constants;
		constants.radiance = light->getColor();
		constants.roughness = mRoughness;
		constants.metallic = mMetallic;

		auto dir = light->getDirection();
		dir.Normalize();
		auto pos = light->getNode()->getRealPosition();
		if (light->getType() == Scene::Light::LT_DIR)
			constants.lightpos = { dir.x, dir.y,dir.z ,0 };
		else
			constants.lightpos = { pos.x, pos.y,pos.z ,0 };

		auto campos = cam->getNode()->getRealPosition();
		constants.cameraPos = { campos.x, campos.y, campos.z, 1.0f };
		Matrix viewproj = cam->getViewMatrix() * cam->getProjectionMatrix();
		constants.invertViewPorj = viewproj.Invert().Transpose();

		mConstants.lock()->blit(&constants, sizeof(constants));

		mQuad.setRenderTarget(rt);
		mQuad.setTextures({ mAlbedo, mNormal, mDepth });
		mQuad.setPixelShader(mPSs[light->getType()]);
		mQuad.setSamplers({ mLinear, mPoint });
		mQuad.setConstant(mConstants);

		D3D11_BLEND_DESC desc = { 0 };

		desc.RenderTarget[0] = {
			TRUE,
			D3D11_BLEND_ONE,
			D3D11_BLEND_ONE,
			D3D11_BLEND_OP_ADD,
			D3D11_BLEND_ONE,
			D3D11_BLEND_ONE,
			D3D11_BLEND_OP_ADD,
			D3D11_COLOR_WRITE_ENABLE_ALL
		};

		mQuad.setBlend(desc);
		mQuad.draw();
	});


}
