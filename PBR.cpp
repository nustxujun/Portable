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
	this->set("roughness", { {"type","set"}, {"value",1.0f},{"min","0.01"},{"max",1.0f},{"interval", "0.01"} });
	this->set("metallic", { {"type","set"}, {"value",0.9f},{"min","0"},{"max",1.0f},{"interval", "0.01"} });
	this->set("ambient", { {"type","set"}, {"value",0},{"min","0"},{"max","1"},{"interval", "0.001"} });


	const std::vector<const char*> definitions = { "POINT", "DIR", "SPOT", "TILED", "CLUSTERED" };
	for (int i = 0; i < definitions.size(); ++i)
	{

		std::vector<D3D10_SHADER_MACRO> macros = { {definitions[i],"1"} };
		macros.push_back({ NULL, NULL });
		auto blob = r->compileFile("hlsl/lighting.hlsl", "main", "ps_5_0", macros.data());
		mPSs.push_back( r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize()) );
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
		mLightVolumeVS = r->createVertexShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());
	}

	mLightVolumeConstants = r->createBuffer(sizeof(LightVolumeConstants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	mAlbedo = getShaderResource("albedo");
	mNormal = getShaderResource("normal");
	mDepth = getDepthStencil("depth");
	mDepthLinear = getShaderResource("depthlinear");


}

PBR::~PBR()
{
}

void PBR::render(Renderer::Texture2D::Ptr rt) 
{
	if (has("tiled") || has("clustered"))
		renderNormal(rt);
	else
		renderLightVolumes(rt);
	

}

void PBR::init(const Vector3 & cluster, const std::vector<Renderer::Texture2D::Ptr>& shadows)
{ 
	mCluster = cluster; 
	mShadowTextures = shadows; 

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
	constants.invertViewPorj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.view = cam->getViewMatrix().Transpose();
	
	constants.nearZ = cam->getNear();
	constants.farZ = cam->getFar();
	constants.clustersize = mCluster;
	constants.numdirs = getValue<int>("numdirs");
	constants.ambient = getValue<float>("ambient");
	constants.campos = cam->getNode()->getRealPosition();

	mConstants.lock()->blit(&constants, sizeof(constants));

	auto quad = getQuad();
	quad->setRenderTarget(rt);
	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		mAlbedo, mNormal, mDepthLinear , 
		getShaderResource("material"),
		getShaderResource("irradinace"),
		getShaderResource("prefiltered"),
		getShaderResource("lut"),
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

	std::vector<Renderer::ShaderResource::Ptr> srvs = {
	mAlbedo, mNormal, mDepthLinear ,
	getShaderResource("material"),
	getShaderResource("irradinace"),
	getShaderResource("prefiltered"),
	getShaderResource("lut"),
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

	renderer->setTextures(srvs);



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

	Constants constants;
	constants.invertViewPorj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.view = cam->getViewMatrix().Invert().Transpose();

	constants.roughness = getValue<float>("roughness");
	constants.metallic = getValue<float>("metallic");
	constants.width = renderer->getWidth();
	constants.height = renderer->getHeight();
	constants.campos = cam->getNode()->getRealPosition();
	constants.ambient = getValue<float>("ambient");

	mConstants.lock()->blit(&constants, sizeof(constants));
	renderer->setPSConstantBuffers({ mConstants });
	// point
	{
		auto srv = getShaderResource("pointlights")->getView();
		renderer->getContext()->VSSetShaderResources(0, 1,&srv);
		consts.stride = 2; // pos, color
		mLightVolumeConstants.lock()->blit(&consts, sizeof(consts));
		renderer->setVSConstantBuffers({ mLightVolumeConstants });

		int count = getValue<int>("numpoints");

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

		// 1. draw front face and back face in depth and stencil
		renderer->clearStencil(mDepth,1);
		renderer->setDepthStencilState(dsdesc, 1);
		renderer->setPixelShader({});
		renderer->setRenderTargets({}, mDepth);
		context->DrawIndexedInstanced(rend.numIndices, count, 0, 0, 0);
		// 2  render back face and test stencil(only draw pixel in the range of light volume)
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

	// dir
	{
		auto quad = getQuad();
		quad->setRenderTarget(rt);
		quad->setTextures(srvs);
		quad->setPixelShader(mPSs[Scene::Light::LT_DIR]);

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
	renderer->removeShaderResourceViews();
	renderer->removeRenderTargets();
}
