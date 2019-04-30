#include "Lighting.h"


Lighting::Lighting(Renderer::Ptr renderer) :mRenderer(renderer)
{
	auto shader = renderer->compileFile("lighting.fx","","fx_5_0");
	mEffect = renderer->createEffect((*shader)->GetBufferPointer(), (*shader)->GetBufferSize());

	mLightingMap = renderer->createRenderTarget(renderer->getWidth(), renderer->getHeight(), DXGI_FORMAT_R8G8B8A8_UNORM);
}


void Lighting::prepare()
{
	auto e = mEffect.lock();
	if (e == nullptr)
		return;

}


Lighting::~Lighting()
{

}
