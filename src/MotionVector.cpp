#include "MotionVector.h"

void MotionVector::init()
{
	mName = "motion vector";
	mEffect = getRenderer()->createEffect("hlsl/motionvector.fx");
	auto cam = getCamera();
	auto vp = cam->getViewport();
	mMotionVector = getRenderer()->createRenderTarget(vp.Width, vp.Height, DXGI_FORMAT_R16G16_FLOAT);
	addShaderResource("motionvector", mMotionVector);
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = getRenderer()->createLayout(layout, ARRAYSIZE(layout));

}

void MotionVector::render(Renderer::Texture2D::Ptr rt)
{
	auto depth = getDepthStencil("depth");
	auto renderer = getRenderer();
	auto scene = getScene();
	auto cam = scene->createOrGetCamera("main");

	renderer->clearRenderTarget(mMotionVector, { 0,0,0,0 });
	renderer->setRenderTarget(mMotionVector, depth);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();
	renderer->clearStencil(depth, 0);

	renderer->setDepthStencilState("depth_write_less_equal");
	//renderer->setSampler(mPoint);

	renderer->setDefaultRasterizer();
	renderer->setViewport(cam->getViewport());

	auto e = mEffect.lock();
	auto view = cam->getViewMatrix();
	e->getVariable("View")->AsMatrix()->SetMatrix((const float*)& view);
	e->getVariable("lastView")->AsMatrix()->SetMatrix((const float*)& mLastViewMatrix);
	mLastViewMatrix = view;
	auto proj = cam->getProjectionMatrix();

	// no jitter (may jittered from taa);
	proj._21 = 0;
	proj._22 = 0;

	e->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)& proj);
	e->setTech("dynamic");
	scene->visitRenderables([e, renderer, this](const Renderable& r)
	{
		if (mLastWorld.find(r.id) != mLastWorld.end())
			e->getVariable("lastWorld")->AsMatrix()->SetMatrix((const float*)&mLastWorld[r.id]);
		else
			e->getVariable("lastWorld")->AsMatrix()->SetMatrix((const float*)&Matrix::Identity);
		mLastWorld[r.id] = r.tranformation;
		e->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
		e->render(renderer, [e, renderer, this, r](auto pass)
		{
			renderer->setLayout(mLayout.lock()->bind(pass));
			renderer->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			renderer->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);
			renderer->getContext()->DrawIndexed(r.numIndices, 0, 0);
		});

	}, [](Scene::Entity::Ptr e) {
		//return !e->isStatic();
		return true;
	});
}
