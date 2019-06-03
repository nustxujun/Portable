#include "PBR.h"
#include "GeometryMesh.h"
#include <sstream>
PBR::PBR(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr set,
	Pipeline* p) :
	Pipeline::Stage(r, s, q,set, p)
{
	mName = "pbr lighting";
	this->set("roughness", { {"type","set"}, {"value",0.5f},{"min","0.01"},{"max",1.0f},{"interval", "0.01"} });
	this->set("metallic", { {"type","set"}, {"value",0.5f},{"min","0"},{"max",1.0f},{"interval", "0.01"} });
	this->set("radiance", { {"type","set"}, {"value",1},{"min","1"},{"max",1000},{"interval", "1"} });

	const std::array<const char*, 3> definitions = { "POINT", "DIR", "SPOT" };
	for (int i = 0; i < definitions.size(); ++i)
	{

		std::vector<D3D10_SHADER_MACRO> macros = { {definitions[i],""} };
		if (has("tiled") )
			macros.push_back({ "TILED", "1"});
		else if (has("clustered"))
			macros.push_back({ "CLUSTERED", "1" });

		macros.push_back({ NULL, NULL });
		
		auto blob = r->compileFile("hlsl/lighting.hlsl", "main", "ps_5_0", macros.data());
		mPSs[i] = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}


	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER,NULL, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);


	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32_UINT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA,1},
	};
	mLightVolumeLayout = r->createLayout(layout, ARRAYSIZE(layout));

	{
		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "1";
		params["resolution"] = "5";
		mLightVolumes[Scene::Light::LT_POINT] = Mesh::Ptr(new GeometryMesh(params, r));


		size_t numlights = s->getNumLights();
		std::vector<unsigned int> indices(numlights);
		for (size_t i = 0; i < indices.size(); ++i)
			indices[i] = i;

		D3D11_SUBRESOURCE_DATA data = {0};
		data.pSysMem = indices.data();
		mLightVolumesInstances[Scene::Light::LT_POINT] = r->createBuffer(sizeof(float) * numlights , D3D11_BIND_VERTEX_BUFFER, &data);
	}

	{
		auto blob = r->compileFile("hlsl/lightvolume_vs.hlsl", "main", "vs_5_0");
		mLightVolumeVS = r->createVertexShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}

	mLightVolumeConstants = r->createBuffer(sizeof(LightVolumeConstants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	mAlbedo = getShaderResource("albedo");
	mNormal = getShaderResource("normal");
	mDepth = getDepthStencil("depth");
	mDepthLinear = getShaderResource("depthlinear");
	mLightsBuffer = getBuffer("lights");

	if (has("tiled"))
		mLightsIndex = getShaderResource("lightsindex");
}

PBR::~PBR()
{
}

void PBR::render(Renderer::Texture2D::Ptr rt) 
{
	updateLights();
	if (has("tiled") || has("clustered"))
		renderNormal(rt);
	else
		renderLightVolumes(rt);
	

}

void PBR::updateLights()
{
	auto cam = getScene()->createOrGetCamera("main");
	Matrix view = cam->getViewMatrix();
	D3D11_MAP map = D3D11_MAP_WRITE_DISCARD;
	D3D11_MAPPED_SUBRESOURCE subresource;
	auto context = getRenderer()->getContext();
	auto buffer = mLightsBuffer.lock();
	context->Map(*buffer, 0, map, 0,&subresource);
	char* data = (char*)subresource.pData;
	float range = 1000.0f;
	if (has("lightRange"))
		range = getValue<float>("lightRange");
	float radiance = 1.0f;
	if (has("radiance"))
		radiance = getValue<float>("radiance");
	getScene()->visitLights([view, range, &data, radiance](Scene::Light::Ptr light)
	{
		Vector3 pos = light->getNode()->getRealPosition();
		Vector4 vpos = Vector4::Transform({ pos.x, pos.y, pos.z, 1 }, view);
		vpos.w = range;
		memcpy(data, &vpos, sizeof(vpos));
		data += sizeof(vpos);
		Vector3 color = light->getColor();
		color *= radiance;
		memcpy(data, &Vector4(color.x, color.y, color.z,1.0f),sizeof(Vector4));
		data += sizeof(Vector4);
	});
	context->Unmap(*buffer, 0);
}

void PBR::renderNormal(Renderer::Texture2D::Ptr rt)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto cam = getScene()->createOrGetCamera("main");



	Constants constants;

	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	auto desc = rt.lock()->getDesc();
	constants.width = desc.Width;
	constants.height = desc.Height;
	constants.maxLightsPerTile =  getScene()->getNumLights();
	constants.tilePerline = ((desc.Width + 16 - 1) & ~15) / 16;
	constants.invertPorj = cam->getProjectionMatrix().Invert().Transpose();
	constants.nearZ = cam->getNear();
	constants.farZ = cam->getFar();
	constants.clustersize = mCluster;

	mConstants.lock()->blit(&constants, sizeof(constants));

	auto quad = getQuad();
	quad->setRenderTarget(rt);
	if (has("tiled"))
		quad->setTextures({ mAlbedo, mNormal, mDepthLinear , mLightsBuffer, mLightsIndex});
	else if (has("clustered"))
	{
		quad->setTextures({ mAlbedo, mNormal, mDepthLinear , mLightsBuffer, getShaderResource("lightindices"), getShaderResource("clusteredlights") });
	}
	quad->setPixelShader(mPSs[Scene::Light::LT_POINT]);
	quad->setSamplers({ mLinear, mPoint });
	quad->setConstant(mConstants);

	D3D11_BLEND_DESC blend = { 0 };

	blend.RenderTarget[0] = {
		TRUE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};

	quad->setBlend(blend);

	quad->draw();
}



void PBR::renderLightVolumes(Renderer::Texture2D::Ptr rt)
{
	auto renderer = getRenderer();
	renderer->setViewport({ 0,0, (float)renderer->getWidth(), (float)renderer->getHeight(), 0, 1.0f });
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultRasterizer();
	renderer->setDefaultDepthStencilState();
	renderer->setSamplers({ mLinear, mPoint });
	renderer->setTextures({ mAlbedo, mNormal, mDepthLinear ,mLightsBuffer });
	ID3D11ShaderResourceView* srvs = *mLightsBuffer.lock();
	renderer->getContext()->VSSetShaderResources(0, 1, &srvs);

	D3D11_BLEND_DESC blend = { 0 };
	blend.RenderTarget[0] = {
		TRUE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};
	renderer->setBlendState(blend);



	auto cam = getScene()->createOrGetCamera("main");
	LightVolumeConstants consts;

	Matrix view = cam->getViewMatrix();
	consts.View = view.Transpose();
	consts.Projection = cam->getProjectionMatrix().Transpose();
	mLightVolumeConstants.lock()->blit(&consts, sizeof(consts));
	renderer->setVSConstantBuffers({ mLightVolumeConstants });

	Constants constants;
	constants.invertPorj = cam->getProjectionMatrix().Invert().Transpose();
	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	constants.width = renderer->getWidth();
	constants.height = renderer->getHeight();
	mConstants.lock()->blit(&constants, sizeof(constants));
	renderer->setPSConstantBuffers({ mConstants });
	// point
	{
		int count = getScene()->getNumLights();
		if (has("numLights"))
			count = getValue<int>("numLights");


		auto instances = mLightVolumesInstances[Scene::Light::LT_POINT];
		auto context = renderer->getContext();
		Mesh::Ptr mesh = mLightVolumes[Scene::Light::LT_POINT];
		auto layout = mLightVolumeLayout;
		auto rend = mesh->getMesh(0);
		ID3D11Buffer* vbs[] = { *rend.vertices.lock(),*instances.lock() };
		UINT stride[] = { rend.layout.lock()->getSize(),4 };
		UINT offset[] = { 0, 0 };

		renderer->setVertexShader(mLightVolumeVS);
		renderer->setIndexBuffer(rend.indices, DXGI_FORMAT_R32_UINT, 0);
		renderer->setVertexBuffers( {rend.vertices, instances}, stride, offset);
		renderer->setLayout(layout.lock()->bind(mLightVolumeVS));

		D3D11_RASTERIZER_DESC rasterDesc;
		rasterDesc.AntialiasedLineEnable = false;
		rasterDesc.CullMode = D3D11_CULL_NONE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = false;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		renderer->setRasterizer(rasterDesc);

		D3D11_DEPTH_STENCIL_DESC dsdesc =
		{
			TRUE,
			D3D11_DEPTH_WRITE_MASK_ZERO,
			D3D11_COMPARISON_LESS,
			TRUE,
			D3D11_DEFAULT_STENCIL_READ_MASK,
			D3D11_DEFAULT_STENCIL_WRITE_MASK,
			{
				D3D11_STENCIL_OP_KEEP,
				D3D11_STENCIL_OP_INCR,
				D3D11_STENCIL_OP_KEEP,
				D3D11_COMPARISON_ALWAYS,
			},
			{
				D3D11_STENCIL_OP_KEEP,
				D3D11_STENCIL_OP_DECR,
				D3D11_STENCIL_OP_KEEP,
				D3D11_COMPARISON_ALWAYS
			}
		};

		mDepth.lock()->clearStencil(1);
		renderer->setDepthStencilState(dsdesc, 1);
		renderer->setPixelShader({});
		renderer->setRenderTargets({}, mDepth);
		context->DrawIndexedInstanced(rend.numIndices, count, 0, 0, 0);

		dsdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsdesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsdesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsdesc.DepthEnable = false;
		dsdesc.StencilEnable = true;
		renderer->setDepthStencilState(dsdesc, 1);

		rasterDesc.CullMode = D3D11_CULL_FRONT;
		renderer->setRasterizer(rasterDesc);
		renderer->setPixelShader(mPSs[Scene::Light::LT_POINT]);
		renderer->setRenderTarget(rt, mDepth);
		context->DrawIndexedInstanced(rend.numIndices, count, 0, 0, 0);

	}

	renderer->removeShaderResourceViews();
	renderer->removeRenderTargets();
}
