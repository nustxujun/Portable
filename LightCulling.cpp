#include "LightCulling.h"

LightCulling::LightCulling(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr set,
	Pipeline * p,
	Renderer::Texture::Ptr depthBounds,
	Renderer::Buffer::Ptr lightsindex) :
	Pipeline::Stage(r, s, q,set, p), mComputer(r),
	mDepthBounds(depthBounds),
	mLightsOutput (lightsindex)
{
	mName = "cull lights";
	auto blob = r->compileFile("hlsl/lightculling.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);

	this->set("tiled", { {"value",true } });
	
	
	this->set("maxLightsPerTile", { {"value", 100}, {"min", 1}, {"max", 100}, {"interval", 1}, {"type","set"} });


	auto desc = depthBounds.lock()->getDesc();


}

void LightCulling::render(Renderer::Texture::Ptr rt) 
{

	Constants consts;
	auto cam = getScene()->createOrGetCamera("main");
	consts.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	const Matrix& view = cam->getViewMatrix();
	consts.numLights = 0;
	getScene()->visitLights([&consts, &view,this](Scene::Light::Ptr l)
	{
		const Vector3 pos = l->getNode()->getRealPosition();
		Vector4 wpos = { pos.x, pos.y, pos.z, 1 };
		wpos = Vector4::Transform(wpos, view);
		consts.lights[consts.numLights++] = { wpos.x, wpos.y ,wpos.z ,getValue<float>("lightRange") };
	});

	consts.numLights = getValue<int>("numLights");
	auto desc = mDepthBounds.lock()->getDesc();
	consts.texelwidth = 1.0f / (float)desc.Width;
	consts.texelheight = 1.0f/ (float)desc.Height;
	consts.maxLightsPerTile = getValue<int>("maxLightsPerTile");
	consts.tilePerline = desc.Width;

	mConstants.lock()->blit(&consts, sizeof(Constants));

	mComputer.setConstants({ mConstants });
	mComputer.setInputs({ mDepthBounds });
	mComputer.setOuputs({ mLightsOutput });
	mComputer.setShader(mCS);
	mComputer.compute(desc.Width, desc.Height, 1);

	//Quad quad(getRenderer());
	//quad.setRenderTarget(rt);
	//quad.drawTexture(mOutput, false);
}
