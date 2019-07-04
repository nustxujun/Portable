#include "MotionBlur.h"

void MotionBlur::init()
{
	mName = "camera motion blur";
	mPS = getRenderer()->createPixelShader("hlsl/motionblur.hlsl");
	mConstants = getRenderer()->createConstantBuffer(sizeof(Constants));
}

void MotionBlur::render(Renderer::Texture2D::Ptr rt)
{
	if (mFrame.expired())
	{
		mFrame = getRenderer()->createTexture(rt->getDesc());
	}
	Constants constants;
	auto cam = getScene()->createOrGetCamera("main");
	auto proj = cam->getProjectionMatrix();
	auto view = cam->getViewMatrix();
	constants.invertViewProj = (view * proj).Invert().Transpose();
	constants.lastView = mLastViewMatrix.Transpose();
	constants.proj = proj.Transpose();
	mConstants.lock()->blit(constants);

	mLastViewMatrix = view;

	auto quad = getQuad();
	quad->setDefaultBlend(false);
	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setPixelShader(mPS);
	quad->setRenderTarget(mFrame);
	quad->setConstant(mConstants);
	quad->setTextures({
		rt,
		getShaderResource("depth")
		});
	quad->draw();

	mFrame->swap(rt);


}
