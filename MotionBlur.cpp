#include "MotionBlur.h"

void MotionBlur::init()
{
	mName = "camera motion blur";
	mPS = getRenderer()->createPixelShader("hlsl/motionblur.hlsl");
	mConstants = getRenderer()->createConstantBuffer(sizeof(Constants));

	mMotionVectorEffect = getRenderer()->createEffect("hlsl/motionvector.fx");
	auto cam = getCamera();
	auto vp = cam->getViewport();
	mMotionVector = getRenderer()->createRenderTarget(vp.Width, vp.Height, DXGI_FORMAT_R32G32_FLOAT);
	addShaderResource("motionvector", mMotionVector);
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = getRenderer()->createLayout(layout, ARRAYSIZE(layout));
}

void MotionBlur::render(Renderer::Texture2D::Ptr rt)
{
	renderMotionVector();

	if (mFrame.expired())
	{
		mFrame = getRenderer()->createTexture(rt->getDesc());
	}
	Constants constants;
	auto cam = getCamera();
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
		getShaderResource("depth"),
		mMotionVector
		});
	quad->draw();

	mFrame->swap(rt);


}

void MotionBlur::renderMotionVector()
{
	auto depth = getDepthStencil("depth");
	auto renderer = getRenderer();
	auto scene = getScene();
	auto cam = scene->createOrGetCamera("main");

	renderer->setRenderTarget(mMotionVector, depth);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->setDefaultDepthStencilState();
	//renderer->setSampler(mPoint);

	renderer->setDefaultRasterizer();
	renderer->setViewport(cam->getViewport());

	auto e = mMotionVectorEffect.lock();
	e->getVariable("View")->AsMatrix()->SetMatrix((const float*)& cam->getViewMatrix());
	e->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)& cam->getProjectionMatrix());
	e->setTech("dynamic");

	scene->visitRenderables([e, renderer, this](const Renderable& r)
	{
		if (mLastWorld.find(r.id) != mLastWorld.end())
			e->getVariable("lastWorld")->AsMatrix()->SetMatrix((const float*)&mLastWorld[r.id]);
		else
			e->getVariable("lastWorld")->AsMatrix()->SetMatrix((const float*)&Matrix::Identity);
		mLastWorld[r.id] = r.tranformation;
		e->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
		e->render(renderer, [e, renderer, this,r ](auto pass)
		{
			renderer->setLayout(mLayout.lock()->bind(pass));
			renderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			renderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
			renderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});
		
	}, [](Scene::Entity::Ptr e) {
		return true;
	});


}
