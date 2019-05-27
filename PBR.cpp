#include "PBR.h"

PBR::PBR(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr set,
	Pipeline* p,
	Renderer::Texture::Ptr a,
	Renderer::Texture::Ptr n,
	Renderer::DepthStencil::Ptr d,
	Renderer::Buffer::Ptr lightsindex) :
	Pipeline::Stage(r, s, q,set, p),
	mAlbedo(a),
	mNormal(n),
	mDepth(d),
	mLightsIndex(lightsindex)
{
	mName = "pbr lighting";
	this->set("roughness", { {"type","set"}, {"value",0.5f},{"min","0.01"},{"max",1.0f},{"interval", "0.01"} });
	this->set("metallic", { {"type","set"}, {"value",0.5f},{"min","0"},{"max",1.0f},{"interval", "0.01"} });
	this->set("radiance", { {"type","set"}, {"value",1},{"min","1"},{"max",1000},{"interval", "1"} });
	this->set("lightRange", { {"value", 100}, {"min", 1}, {"max", 1000}, {"interval", 1}, {"type","set"} });

	const std::array<const char*, 3> definitions = { "POINT", "DIR", "SPOT" };
	for (int i = 0; i < definitions.size(); ++i)
	{

		std::vector<D3D10_SHADER_MACRO> macros = { {definitions[i],""} };
		if (has("tiled") && getValue<bool>("tiled"))
			macros.push_back({ "TILED", ""});

		macros.push_back({ NULL, NULL });
		
		auto blob = r->compileFile("hlsl/pbr.hlsl", "main", "ps_5_0", macros.data());
		mPSs[i] = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}


	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER,NULL, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

PBR::~PBR()
{
}

void PBR::render(Renderer::Texture::Ptr rt) 
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto cam = getScene()->createOrGetCamera("main");



	Constants constants;
	int index = 0;
	Matrix view = cam->getViewMatrix();
	getScene()->visitLights([&constants, &index,view,this](Scene::Light::Ptr light) {
		Vector3 color = light->getColor();
		color *= getValue<float>("radiance");
		constants.lightscolor[index] = {color.x,color.y,color.z,1};

		auto dir = light->getDirection();
		dir.Normalize();
		auto pos = light->getNode()->getRealPosition();
		if (light->getType() == Scene::Light::LT_DIR)
			constants.lightspos[index] = { dir.x, dir.y,dir.z ,0 };
		else
			constants.lightspos[index] = { pos.x, pos.y,pos.z , 1};

		constants.lightspos[index] = Vector4::Transform(constants.lightspos[index], view);
		if (light->getType() != Scene::Light::LT_DIR)
			constants.lightspos[index].w = getValue<float>("lightRange");
		index++;
	});

	constants.numLights = index;
	if (has("numLights"))
		constants.numLights = std::min(index, getValue<int>("numLights"));
	
	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	auto desc = mAlbedo.lock()->getDesc();
	constants.width = desc.Width;
	constants.height = desc.Height;
	constants.maxLightsPerTile = std::min(getValue<int>("maxLightsPerTile"), constants.numLights);
	constants.tilePerline = ((desc.Width + 16 - 1) & ~15) / 16;


	constants.invertPorj = cam->getProjectionMatrix().Invert().Transpose();

	mConstants.lock()->blit(&constants, sizeof(constants));

	auto quad = getQuad();
	quad->setRenderTarget(rt);
	quad->setTextures({ mAlbedo, mNormal, mDepth ,mLightsIndex});
	quad->setPixelShader(mPSs[Scene::Light::LT_POINT]);
	quad->setSamplers({ mLinear, mPoint });
	quad->setConstant(mConstants);

	D3D11_BLEND_DESC blend = { 0 };

	blend.RenderTarget[0] = {
		TRUE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};

	quad->setBlend(blend);
	quad->draw();

}
