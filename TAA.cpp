#include "TAA.h"

void TAA::init()
{
	mBlend = getRenderer()->createPixelShader("hlsl/taa.hlsl", "blendHistory");

	auto cam = getCamera();
	mConstants = getRenderer()->createConstantBuffer(sizeof(Constants));
}

void TAA::render(Renderer::Texture2D::Ptr rt)
{

	blend(rt);
	offsetCamera();
}

void TAA::offsetCamera()
{
	auto cam = getCamera();
	auto vp = cam->getViewport();

	static int frame = 0;
	// http://en.wikipedia.org/wiki/Halton_sequence
	const static float halton[8][2] = {
		{ 1.0f / 2.0f, 1.0f / 3.0f },
		{ 1.0f / 4.0f, 2.0f / 3.0f },
		{ 3.0f / 4.0f, 1.0f / 9.0f },
		{ 1.0f / 8.0f, 4.0f / 9.0f },
		{ 5.0f / 8.0f, 7.0f / 9.0f },
		{ 3.0f / 8.0f, 2.0f / 9.0f },
		{ 7.0f / 8.0f, 5.0f / 9.0f },
		{ 1.0f / 16.0f, 8.0f / 9.0f },
	};
	frame = (frame + 1) & 0x7;
	mJitter = { halton[frame][0] / (float)vp.Width, halton[frame][1] / (float)vp.Height };
	cam->setProjectionOffset({ mJitter.x, mJitter.y ,0 });

	
}

void TAA::blend(Renderer::Texture2D::Ptr rt)
{
	auto renderer = getRenderer();

	if (!mHistory)
	{
		mHistory = renderer->createTemporaryRT(rt->getDesc());
		renderer->getContext()->CopySubresourceRegion(mHistory->get()->getTexture(), 0, 0, 0, 0, rt->getTexture(), 0, NULL);
	}
	auto quad = getQuad();

	quad->setDefaultBlend(false);
	quad->setDefaultPixelShader();
	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setPixelShader(mBlend);

	auto motionvec = getShaderResource("motionvector");
	quad->setTextures({ rt, mHistory->get() ,motionvec });
	auto frame = getRenderer()->createTemporaryRT(rt->getDesc());
	quad->setRenderTarget(frame->get());

	Constants c;
	c.jitter = mJitter;
	c.texsize = {1.0f/ rt->getDesc().Width,1.0f / rt->getDesc().Height };
	mConstants->blit(c);
	quad->setConstant(mConstants);

	quad->draw();
	mHistory.swap(frame);
	renderer->getContext()->CopySubresourceRegion(rt->getTexture(), 0, 0, 0, 0, mHistory->get()->getTexture(), 0, NULL);
}
