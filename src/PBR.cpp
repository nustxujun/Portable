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
	this->set("roughness", { {"type","set"}, {"value",1},{"min",0.01},{"max",1.0f},{"interval", "0.01"} });
	this->set("metallic", { {"type","set"}, {"value",1},{"min",0},{"max",1.0f},{"interval", "0.01"} });
	this->set("ambient", { {"type","set"}, {"value",0},{"min",0},{"max",1},{"interval", "0.001"} });

	const std::vector<const char*> definitions = { "POINT", "DIR", "SPOT", "TILED", "CLUSTERED" };
	for (int i = 0; i < definitions.size(); ++i)
	{

		std::vector<D3D10_SHADER_MACRO> macros = { {definitions[i],"1"} };
		if (has("translucency"))
			macros.push_back({ "TRANSMITTANCE", "1" });
		macros.push_back({ NULL, NULL });
		auto blob = r->compileFile("hlsl/lighting.hlsl", "main", "ps_5_0", macros.data());
		mPSs.push_back( r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize()) );
	}


	mLinear = r->createSampler("liear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	mPoint = r->createSampler("point_clamp", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
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
		params["resolution"] = "10";
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
		mLightVolumeVS = r->createVertexShader(blob->GetBufferPointer(), blob->GetBufferSize());
	}

	mLightVolumeConstants = r->createBuffer(sizeof(LightVolumeConstants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	mAlbedo = getShaderResource("albedo");
	mNormal = getShaderResource("normal");
	mDepth = getDepthStencil("depth");

	auto vp = getCamera()->getViewport();

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = vp.Width;
	desc.Height = vp.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	mDepthCopy = getRenderer()->createTexture(desc);


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

void PBR::init(const Vector3 & cluster, const std::vector<Renderer::Texture2D::Ptr>& shadows)
{ 
	mCluster = cluster; 
	mShadowTextures = shadows; 

	auto renderer = getRenderer();
	constexpr auto MAX_NUM_LIGHTS = 1024;
	auto pointlights = renderer->createRWBuffer(sizeof(Vector4) * 2 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	addShaderResource("pointlights", pointlights);
	addBuffer("pointlights", pointlights);

	auto spotlights = renderer->createRWBuffer(sizeof(Vector4) * 3 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	addShaderResource("spotlights", spotlights);
	addBuffer("spotlights", spotlights);

	auto dirlights = renderer->createRWBuffer(sizeof(Vector4) * 2 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	addShaderResource("dirlights", dirlights);
	addBuffer("dirlights", dirlights);




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

	renderer->createDepthStencilState("depth_write_less_stencil_front_incr_back_decr",dsdesc);

	dsdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsdesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
	dsdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsdesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
	dsdesc.DepthEnable = false;
	dsdesc.StencilEnable = true;

	renderer->createDepthStencilState("stencil_front_not_equal_back_not_equal",dsdesc);

}

void PBR::updateLights()
{
	auto scene = getScene();
	std::vector<Scene::Light::Ptr> lights[3];
	std::map<void*, int> shadowindices;
	int index = 1;
	scene->visitLights([&lights, &shadowindices, &index, this](Scene::Light::Ptr l) {
		lights[l->getType()].push_back(l);
		if (l->isCastingShadow() && index <= mShadowTextures.size())
			shadowindices[&(*l)] = index++;
	});

	for (int i = 0; i < 3; ++i)
		mNumLights[i] = lights[i].size();

	std::vector<Renderer::Buffer::Ptr> lightbuffers = 
	{
		getBuffer("pointlights"),
		getBuffer("dirlights"),
		getBuffer("spotlights"),
	};

	auto renderer = getRenderer();
	auto context = renderer->getContext();

	auto copy = [](char*& data, auto value)
	{
		memcpy(data, &value, sizeof(value));
		data += sizeof(value);
	};


	for (int type = 0; type < 3; ++type)
	{
		auto buffer = lightbuffers[type]->getBuffer();
		D3D11_MAPPED_SUBRESOURCE subresource;
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
		char* data = (char*)subresource.pData;

		for (int i = 0; i < lights[type].size(); ++i)
		{
			auto light = lights[type][i];
			auto shadowindex = shadowindices[&(*light)];

			float range = light->getRange();
			if (has("lightRange"))
				range = getValue<float>("lightRange");
			switch (type)
			{
			case Scene::Light::LT_POINT:
				{
					auto pos = light->getNode()->getRealPosition();
					Vector4 vpos = { pos.x, pos.y, pos.z, range };
					copy(data, vpos);
					auto color = light->getColor();
					if (has("pointradiance"))
						color *= getValue<float>("pointradiance");
					copy(data, Vector4(color.x, color.y, color.z, shadowindex));
				}
				break;
			case Scene::Light::LT_DIR:
				{
					auto dir = light->getDirection();
					copy(data, Vector4(dir.x, dir.y, dir.z, 0));

					Vector3 color = light->getColor();
					if (has("dirradiance"))
						color *= getValue<float>("dirradiance");

					copy(data, Vector4(color.x, color.y, color.z, shadowindex));
				}
				break;
			case Scene::Light::LT_SPOT:
				{
					auto pos = light->getNode()->getRealPosition();
					Vector4 vpos = { pos.x, pos.y, pos.z,range };
					copy(data, vpos);

					auto dir = light->getDirection();
					copy(data, Vector4(dir.x, dir.y, dir.z, std::cos(light->getSpotAngle())));

					auto color = light->getColor();
					if (has("spotradiance"))
						color *= getValue<float>("spotradiance");
					copy(data, Vector4(color.x, color.y, color.z, shadowindex));
				}
				break;
			}
		}
		context->Unmap(buffer, 0);
	}
}



void PBR::renderNormal(Renderer::Texture2D::Ptr rt)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto cam = getCamera();



	Constants constants;

	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	auto desc = rt.lock()->getDesc();
	constants.width = desc.Width;
	constants.height = desc.Height;
	constants.invertViewPorj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.view = cam->getViewMatrix().Transpose();
	
	constants.nearZ = cam->getNear();
	constants.farZ = cam->getFar();
	constants.clustersize = mCluster;
	constants.numdirs = mNumLights[Scene::Light::LT_DIR];
	constants.ambient = getValue<float>("ambient");
	constants.campos = cam->getNode()->getRealPosition();

	mConstants.lock()->blit(&constants, sizeof(constants));

	auto quad = getQuad();
	quad->setRenderTarget(rt);
	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		mAlbedo, mNormal, 
		getShaderResource("depth"),
		getShaderResource("material"),
		//getShaderResource("irradinace"),
		//getShaderResource("prefiltered"),
		//getShaderResource("lut"),
	};
	srvs.resize(30);
	srvs[7] = getShaderResource("pointlights");
	srvs[8] = getShaderResource("spotlights");
	srvs[9] = getShaderResource("dirlights");
	
	
	if (has("tiled") || has("clustered"))
	{
		srvs[10] = getShaderResource("lightindices");
		srvs[11] = getShaderResource("lighttable");
	}

	//srvs[10] = mDefaultShadowTex;
	for (int i = 0; i < mShadowTextures.size(); ++i)
		srvs[i + 20] = mShadowTextures[i];

	quad->setTextures(srvs);


	Renderer::PixelShader::Weak ps;
	if (has("tiled"))
		ps = mPSs[3];
	else if(has("clustered"))
		ps = mPSs[4];

	quad->setPixelShader(ps);

	quad->setSamplers({ mLinear, mPoint });
	quad->setConstant(mConstants);

	quad->setBlendColorAdd();

	quad->draw();
}



void PBR::renderLightVolumes(Renderer::Texture2D::Ptr rt)
{
	auto renderer = getRenderer();
	auto vp = getCamera()->getViewport();
	renderer->setViewport(vp);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	renderer->setDefaultRasterizer();
	renderer->setDefaultDepthStencilState();
	renderer->setSamplers({ mLinear, mPoint });

	ID3D11Resource* res;
	mDepth->getView()->GetResource(&res);
	getRenderer()->getContext()->CopySubresourceRegion(mDepthCopy->getTexture(), 0, 0, 0, 0, res, 0, NULL);
	res->Release();
	std::vector<Renderer::ShaderResource::Ptr> srvs = {
	mAlbedo, mNormal, mDepthCopy ,
	getShaderResource("material"),
	//getShaderResource("irradinace"),
	//getShaderResource("prefiltered"),
	//getShaderResource("lut"),
	};
	srvs.resize(40);
	srvs[7] = getShaderResource("pointlights");
	srvs[8] = getShaderResource("spotlights");
	srvs[9] = getShaderResource("dirlights");


	if (has("tiled") || has("clustered"))
	{
		srvs[10] = getShaderResource("lightindices");
		srvs[11] = getShaderResource("lighttable");
	}

	//srvs[10] = mDefaultShadowTex;
	for (int i = 0; i < mShadowTextures.size(); ++i)
		srvs[i + 20] = mShadowTextures[i];


	renderer->setTextures(srvs);



	renderer->setBlendState("color_add");


	auto cam = getCamera();
	LightVolumeConstants consts;

	Matrix view = cam->getViewMatrix();
	consts.View = view.Transpose();
	consts.Projection = cam->getProjectionMatrix().Transpose();

	Constants constants;
	constants.invertViewPorj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.view = cam->getViewMatrix().Invert().Transpose();

	constants.roughness = 1;
	constants.metallic = 1;
	constants.width = vp.Width;
	constants.height = vp.Height;
	constants.campos = cam->getNode()->getRealPosition();
	constants.ambient = getValue<float>("ambient");
	constants.numdirs = mNumLights[Scene::Light::LT_DIR];
	
	auto aabb = getScene()->getRoot()->getWorldAABB();

	mConstants.lock()->blit(&constants, sizeof(constants));
	renderer->setPSConstantBuffers({ mConstants });

	// point
	int pointcount= mNumLights[Scene::Light::LT_POINT];
	if (pointcount > 0)
	{
		auto srv = getShaderResource("pointlights")->getView();
		renderer->getContext()->VSSetShaderResources(0, 1,&srv);
		consts.stride = 2; // pos, color
		mLightVolumeConstants.lock()->blit(&consts, sizeof(consts));
		renderer->setVSConstantBuffers({ mLightVolumeConstants });


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

		// 1. draw front face and back face in depth and stencil
		renderer->clearStencil(mDepth,1);
		renderer->setDepthStencilState("depth_write_less_stencil_front_incr_back_decr", 1);
		renderer->setPixelShader({});
		renderer->setRenderTargets({}, mDepth);
		context->DrawIndexedInstanced(rend.numIndices, pointcount, 0, 0, 0);
		// 2  render back face and test stencil(only draw pixel in the range of light volume)
		renderer->setDepthStencilState("stencil_front_not_equal_back_not_equal", 1);
		
		//rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
		rasterDesc.CullMode = D3D11_CULL_FRONT;
		renderer->setRasterizer(rasterDesc);
		renderer->setPixelShader(mPSs[Scene::Light::LT_POINT]);
		renderer->setRenderTarget(rt, mDepth);
		context->DrawIndexedInstanced(rend.numIndices, pointcount, 0, 0, 0);

	}

	srvs[2] = getShaderResource("depth");
	// dir
	{
		auto quad = getQuad();
		quad->setRenderTarget(rt);
		quad->setTextures(srvs);
		quad->setPixelShader(mPSs[Scene::Light::LT_DIR]);
		quad->setDefaultViewport();
		quad->setSamplers({ mLinear, mPoint });
		quad->setConstant(mConstants); 

		quad->setBlendColorAdd();
		quad->draw();
	}
	renderer->removeShaderResourceViews();
	renderer->removeRenderTargets();
}
