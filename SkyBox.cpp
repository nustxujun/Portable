#include "SkyBox.h"
#include "GeometryMesh.h"
#include "ImageProcessing.h"
void SkyBox::init(const std::array<std::string,6> & tex)
{
	init(true);
	mSkyTex = getRenderer()->createTextureCube({ tex[0],tex[1] ,tex[2] ,tex[3] ,tex[4] ,tex[5] });
}

void SkyBox::init(const std::string& tex, bool isCubemap)
{
	init(isCubemap);

	if (isCubemap)
	{
		mSkyTex = getRenderer()->createTextureCube(tex);
	}
	else
	{
		mSkyTex = getRenderer()->createTexture(tex,1);
	}
}

void SkyBox::init(bool isCubemap)
{
	mName = "skybox";

	{
		Parameters params;
		params["geom"] = "sphere";
		params["size"] = "1000";
		params["resolution"] = "50";
		mSkyMesh = GeometryMesh::Ptr(new GeometryMesh(params, getRenderer()));
	}

	D3D10_SHADER_MACRO macros[] = { { "EQUIRECT", "0"}, {NULL,NULL} };

	if (!isCubemap)
		macros[0].Definition = "1";
	auto blob = getRenderer()->compileFile("hlsl/cubemap.fx", "", "fx_5_0", macros);
	mEffect = getRenderer()->createEffect(blob->GetBufferPointer(), blob->GetBufferSize());
	mEffect.lock()->setTech("skybox");

	D3D11_INPUT_ELEMENT_DESC modelLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = getRenderer()->createLayout(modelLayout, ARRAYSIZE(modelLayout));
}

void SkyBox::render(Renderer::Texture2D::Ptr rt)
{

	auto renderer = getRenderer();

	renderer->setRenderTargets({ rt }, getDepthStencil("depth"));
	D3D11_DEPTH_STENCIL_DESC desc = {0};
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	
	renderer->setDepthStencilState(desc);
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_FRONT;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	renderer->setRasterizer(rasterDesc);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultBlendState();

	auto e = mEffect.lock();
	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();

	auto cam = getCamera();

	world->SetMatrix((const float*)&cam->getNode()->getTransformation());
	view->SetMatrix((const float*)&cam->getViewMatrix());
	proj->SetMatrix((const float*)&cam->getProjectionMatrix());

	renderer->setViewport(cam->getViewport());

	auto rend = mSkyMesh->getMesh(0);
	renderer->setIndexBuffer(rend.indices, DXGI_FORMAT_R32_UINT, 0);
	renderer->setVertexBuffer(rend.vertices, rend.layout.lock()->getSize(), 0);
	e->render(renderer, [&,this](ID3DX11EffectPass* pass)
	{
		renderer->setTexture(mSkyTex);
		renderer->setLayout(mLayout.lock()->bind(pass));
		renderer->getContext()->DrawIndexed(rend.numIndices, 0, 0);

	});

	renderer->removeRenderTargets();
	renderer->removeShaderResourceViews();
}
