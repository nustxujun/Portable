#include "PBR.h"
#include "GeometryMesh.h"
#include <sstream>
PBR::PBR(
	Renderer::Ptr r,
	Scene::Ptr s,
	Quad::Ptr q,
	Setting::Ptr set,
	Pipeline* p,
	Renderer::Texture::Ptr a,
	Renderer::Texture::Ptr n,
	Renderer::DepthStencil::Ptr d,
	Renderer::Texture::Ptr dl,
	Renderer::Buffer::Ptr lightsindex) :
	Pipeline::Stage(r, s, q,set, p),
	mAlbedo(a),
	mNormal(n),
	mDepth(d),
	mDepthLinear(dl),
	mLightsIndex(lightsindex)
{
	this->set("fillmode", { {"type","set"}, {"value",2},{"min","2"},{"max",3},{"interval", "1"} });
	mName = "pbr lighting";
	this->set("roughness", { {"type","set"}, {"value",0.5f},{"min","0.01"},{"max",1.0f},{"interval", "0.01"} });
	this->set("metallic", { {"type","set"}, {"value",0.5f},{"min","0"},{"max",1.0f},{"interval", "0.01"} });
	this->set("radiance", { {"type","set"}, {"value",1},{"min","1"},{"max",1000},{"interval", "1"} });

	const std::array<const char*, 3> definitions = { "POINT", "DIR", "SPOT" };
	for (int i = 0; i < definitions.size(); ++i)
	{

		std::vector<D3D10_SHADER_MACRO> macros = { {definitions[i],""} };
		if (has("tiled") && getValue<bool>("tiled"))
			macros.push_back({ "TILED", ""});

		macros.push_back({ NULL, NULL });
		
		auto blob = r->compileFile("hlsl/pbr.hlsl", "main", "ps_5_0", macros.data());
		mPSs[i] = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}


	mLinear = r->createSampler("liear_wrap", D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPoint = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mConstants = r->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER,NULL, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);


	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA,1},
		{"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA,1},

	};
	mLightVolumeLayout = r->createLayout(layout, ARRAYSIZE(layout));

	{
		Parameters params;
		params["geom"] = "sphere";
		params["radius"] = "1";
		params["resolution"] = "5";
		mLightVolumes[Scene::Light::LT_POINT] = Mesh::Ptr(new GeometryMesh(params, r));

		int numlights = 100;
		if (has("numlights"))
			numlights = get("numlights")["max"];
		mLightVolumesInstances[Scene::Light::LT_POINT] = r->createBuffer( (sizeof(Vector4) + sizeof(float))* numlights , D3D11_BIND_VERTEX_BUFFER, 0,D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	}

	{
		auto blob = r->compileFile("hlsl/lightvolume_vs.hlsl", "main", "vs_5_0");
		mLightVolumeVS = r->createVertexShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}

	mLightVolumeConstants = r->createBuffer(sizeof(LightVolumeConstants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	mOnlyLighting = r->createProfile();
}

PBR::~PBR()
{
}

void PBR::render(Renderer::Texture::Ptr rt) 
{
	if (has("tiled"))
		renderNormal(rt);
	else
		renderLightVolumes(rt);
	
	std::stringstream ss;
	ss.precision(4);
	ss << mOnlyLighting.lock()->getElapsedTime();
	set("lighting", { {"cost",ss.str().c_str()}, {"type", "stage"} });
}

void PBR::renderNormal(Renderer::Texture::Ptr rt)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto cam = getScene()->createOrGetCamera("main");



	Constants constants;
	int index = 0;
	Matrix view = cam->getViewMatrix();
	getScene()->visitLights([&constants, &index,view,this](Scene::Light::Ptr light) {
		Vector3 color = light->getColor();
		color *= getValue<float>("radiance");
		constants.lightscolor[index] = {color.x,color.y,color.z,1};

		auto dir = light->getDirection();
		dir.Normalize();
		auto pos = light->getNode()->getRealPosition();
		if (light->getType() == Scene::Light::LT_DIR)
			constants.lightspos[index] = { dir.x, dir.y,dir.z ,0 };
		else
			constants.lightspos[index] = { pos.x, pos.y,pos.z , 1};

		constants.lightspos[index] = Vector4::Transform(constants.lightspos[index], view);
		if (light->getType() != Scene::Light::LT_DIR)
			constants.lightspos[index].w = getValue<float>("lightRange");
		index++;
	});

	constants.numLights = index;
	if (has("numLights"))
		constants.numLights = std::min(index, getValue<int>("numLights"));

	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	auto desc = mAlbedo.lock()->getDesc();
	constants.width = desc.Width;
	constants.height = desc.Height;
	constants.maxLightsPerTile = std::min(getValue<int>("maxLightsPerTile"), constants.numLights);
	constants.tilePerline = ((desc.Width + 16 - 1) & ~15) / 16;


	constants.invertPorj = cam->getProjectionMatrix().Invert().Transpose();

	mConstants.lock()->blit(&constants, sizeof(constants));

	auto quad = getQuad();
	quad->setRenderTarget(rt);
	quad->setTextures({ mAlbedo, mNormal, mDepthLinear ,mLightsIndex});
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

	{
		PROFILE(mOnlyLighting);
		quad->draw();
	}
}



void PBR::renderLightVolumes(Renderer::Texture::Ptr rt)
{
	auto renderer = getRenderer();
	renderer->setViewport({ 0,0, (float)renderer->getWidth(), (float)renderer->getHeight(), 0, 1.0f });
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultRasterizer();
	renderer->setDefaultDepthStencilState();
	renderer->setSamplers({ mLinear, mPoint });
	renderer->setTextures({ mAlbedo, mNormal, mDepthLinear });

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
	constants.numLights = 1;
	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	constants.range = getValue<float>("lightRange");
	constants.width = renderer->getWidth();
	constants.height = renderer->getHeight();

	// point
	{
		std::vector<float> lights;
		float range = 10.0f;
		if (has("lightRange"))
			range = get("lightRange")["value"];
		int index = 0;
		float radiance = getValue<float>("radiance");
		getScene()->visitLights([&lights, range,&index,&constants, view, radiance](Scene::Light::Ptr light)
		{
			if (light->getType() != Scene::Light::LT_POINT)
				return;

			auto pos = light->getNode()->getRealPosition();
			Vector4 vpos = Vector4::Transform({ pos.x, pos.y, pos.z, 1 }, view);
			constants.lightspos[index] = { vpos.x, vpos.y, vpos.z, range };
			auto color = light->getColor();
			color *= radiance;
			constants.lightscolor[index] = {color.x, color.y, color.z, 1};

			lights.push_back(pos.x);
			lights.push_back(pos.y);
			lights.push_back(pos.z);
			lights.push_back(range);
			lights.push_back((float)index);
			index++;
		});


		mConstants.lock()->blit(&constants, sizeof(constants));
		renderer->setPSConstantBuffers({ mConstants });

		auto instances = mLightVolumesInstances[Scene::Light::LT_POINT];
		instances.lock()->blit(lights.data(), lights.size() * sizeof(float));

		int instacesCount = lights.size() / 5;
		auto context = renderer->getContext();
		Mesh::Ptr mesh = mLightVolumes[Scene::Light::LT_POINT];
		auto layout = mLightVolumeLayout;
		auto rend = mesh->getMesh(0);
		ID3D11Buffer* vbs[] = { *rend.vertices.lock(),*instances.lock() };
		UINT stride[] = { rend.layout.lock()->getSize(),20 };
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
		rasterDesc.FillMode = getValue<D3D11_FILL_MODE>("fillmode");
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
		context->DrawIndexedInstanced(rend.numIndices, instacesCount, 0, 0, 0);

		dsdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsdesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsdesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
		dsdesc.DepthEnable = false;
		dsdesc.StencilEnable = false;
		renderer->setDepthStencilState(dsdesc, 1);

		rasterDesc.CullMode = D3D11_CULL_FRONT;
		renderer->setRasterizer(rasterDesc);
		renderer->setPixelShader(mPSs[Scene::Light::LT_POINT]);
		renderer->setRenderTarget(rt, mDepth);
		{
			PROFILE(mOnlyLighting);
			context->DrawIndexedInstanced(rend.numIndices, instacesCount, 0, 0, 0);
		}

	}

	renderer->removeShaderResourceViews();
	renderer->removeRenderTargets();
}
