#include "LightCulling.h"

LightCulling::LightCulling(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr set,
	Pipeline * p) :
	Pipeline::Stage(r, s, q,set, p), mComputer(r)
{
	mName = "cull lights";
	auto blob = r->compileFile("hlsl/lightculling.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);

	this->set("tiled", { {"value",true } });
	
	mDepthBounds = getShaderResource("depthbounds");
	mLightsOutput = getUnorderedAccess("lightsindex");
	mLights = getShaderResource("lights");
}

void LightCulling::render(Renderer::Texture::Ptr rt) 
{

	Constants consts;
	auto cam = getScene()->createOrGetCamera("main");
	consts.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	const Matrix& view = cam->getViewMatrix();
	consts.numLights = 0;
	consts.numLights = getValue<int>("numLights");
	consts.texelwidth = 1.0f / (float)mWidth;
	consts.texelheight = 1.0f/ (float)mHeight;
	consts.maxLightsPerTile = getScene()->getNumLights();
	consts.tilePerline = mWidth;

	mConstants.lock()->blit(&consts, sizeof(Constants));

	mComputer.setConstants({ mConstants });
	mComputer.setInputs({ mDepthBounds ,mLights });
	mComputer.setOuputs({ mLightsOutput });
	mComputer.setShader(mCS);
	mComputer.compute(mWidth, mHeight, 1);

	//Quad quad(getRenderer());
	//quad.setRenderTarget(rt);
	//quad.drawTexture(mOutput, false);
}
