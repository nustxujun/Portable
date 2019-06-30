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
	auto blob = r->compileFile("hlsl/tiledlightculling.hlsl", "main", "cs_5_0");
	mCS = r->createComputeShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());

	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER);

	this->set("tiled", { {"value",true } });
	
	mDepthBounds = getShaderResource("depthbounds");
	mLightsOutput = getUnorderedAccess("lightindices");
	mCurIndex = getRenderer()->createRWBuffer(4, 4, DXGI_FORMAT_R32_UINT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

}

void LightCulling::render(Renderer::Texture2D::Ptr rt) 
{
	std::array<UINT, 4> clear = { 0,0,0,0 };
	getRenderer()->clearUnorderedAccess(mCurIndex, clear);

	Constants consts;
	auto cam = getCamera();
	consts.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	const Matrix& view = cam->getViewMatrix();
	consts.view = view.Transpose();
	consts.numPointLights = getValue<int>("numpoints");
	consts.numSpotLights = getValue<int>("numspots");
	consts.texelwidth = 1.0f / (float)mWidth;
	consts.texelheight = 1.0f/ (float)mHeight;

	mConstants.lock()->blit(&consts, sizeof(Constants));

	mComputer.setConstants({ mConstants });
	mComputer.setInputs({ mDepthBounds ,getShaderResource("pointlights"), getShaderResource("spotlights") });
	mComputer.setOuputs({ mCurIndex,  mLightsOutput , getUnorderedAccess("lighttable")});
	mComputer.setShader(mCS);
	mComputer.compute(mWidth, mHeight, 1);

	//Quad quad(getRenderer());
	//quad.setRenderTarget(rt);
	//quad.drawTexture(mOutput, false);
}
