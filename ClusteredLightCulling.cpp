#include "ClusteredLightCulling.h"

void ClusteredLightCulling::init()
{
	mCS = getRenderer()->createComputeShader("hlsl/clusteredlightculling.hlsl","main");
}

void ClusteredLightCulling::render(Renderer::Texture::Ptr rt)
{

}

