#include "LightCulling.h"

LightCulling::LightCulling(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr set,
	Pipeline * p,
	Renderer::Texture::Ptr depthBounds,
	Renderer::Buffer::Ptr lights,
	Renderer::Buffer::Ptr lightsindex) :
	Pipeline::Stage(r, s, q,set, p), mComputer(r),
	mDepthBounds(depthBounds),
	mLightsOutput (lightsindex),
	mLights(lights)
{
	mName = "cull lights";
	auto blob = r->compileFile("hlsl/lightculling.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);

	this->set("tiled", { {"value",true } });
	
	


	auto desc = depthBounds.lock()->getDesc();


}

void LightCulling::render(Renderer::Texture::Ptr rt) 
{

	Constants consts;
	auto cam = getScene()->createOrGetCamera("main");
	consts.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	const Matrix& view = cam->getViewMatrix();
	consts.numLights = 0;
	consts.numLights = getValue<int>("numLights");
	auto desc = mDepthBounds.lock()->getDesc();
	consts.texelwidth = 1.0f / (float)desc.Width;
	consts.texelheight = 1.0f/ (float)desc.Height;
	consts.maxLightsPerTile = getScene()->getNumLights();
	consts.tilePerline = desc.Width;

	mConstants.lock()->blit(&consts, sizeof(Constants));

	mComputer.setConstants({ mConstants });
	mComputer.setInputs({ mDepthBounds ,mLights });
	mComputer.setOuputs({ mLightsOutput });
	mComputer.setShader(mCS);
	mComputer.compute(desc.Width, desc.Height, 1);

	//Quad quad(getRenderer());
	//quad.setRenderTarget(rt);
	//quad.drawTexture(mOutput, false);
}
