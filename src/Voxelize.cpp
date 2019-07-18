#include "Voxelize.h"
#include "VoxelMesh.h"
#include "D3D11Helper.h"
#include "GPUComputer.h"

void Voxelize::init(int size)
{
	mName = "Voxelize";
	mSize = ALIGN(size, 2);

	
	auto renderer = getRenderer();

	D3D11_TEXTURE3D_DESC texdesc;
	texdesc.Width = mSize;
	texdesc.Height = mSize;
	texdesc.Depth = mSize;
	texdesc.MipLevels = 1;
	texdesc.Format = DXGI_FORMAT_R32_UINT;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags = 0;
	texdesc.MiscFlags = 0;
	auto albedo = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelalbedo", albedo);
	addShaderResource("voxelalbedo", albedo);


	texdesc.Format = DXGI_FORMAT_R32_UINT;
	auto normal = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelnormal", normal);
	addShaderResource("voxelnormal", normal);

	texdesc.Format = DXGI_FORMAT_R32_UINT;
	auto material = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelmaterial", material);
	addShaderResource("voxelmaterial", material);

	texdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	auto color = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelcolor", color);
	addShaderResource("voxelcolor", color);
	mColor = color;


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
	mRasterizer = renderer->createOrGetRasterizer(rasterDesc);

	voxelize();

	mInitialize = [this]() {
		voxelLighting();
		visualize();
	};
}

void Voxelize::visualize()
{
	Parameters params;
	auto scene = getScene();
	auto aabb = scene->getRoot()->getWorldAABB();
	auto vec = aabb.second - aabb.first;
	float length = std::max(vec.x, std::max(vec.y, vec.z));
	float scale = length / mSize;
	auto vm = new VoxelMesh(params, getRenderer());
	vm->load(mSize, scale, readTexture3D(mColor), {}, {});

	auto model = scene->createModel("voxels", params, [vm](const Parameters& p)
	{
		return Mesh::Ptr(vm);
	});

	auto center = (aabb.first + aabb.second) * 0.5f;
	Vector3 voxelcenter = { mSize / 2.0f, mSize / 2.0f, mSize / 2.0f };
	center = center - voxelcenter * scale;
	model->getNode()->setPosition(center);
	model->attach(scene->getRoot());
	model->setMask(1);
}


void Voxelize::render(Renderer::Texture2D::Ptr rt)
{
	if (mInitialize) {
		mInitialize();
		mInitialize = nullptr;
	}
}

Renderer::Effect::Ptr Voxelize::getEffect(Material::Ptr mat)
{
	auto ret = mEffect.find(mat->getShaderID());
	if (ret != mEffect.end())
		return ret->second;

	auto macros = mat->generateShaderID();
	macros.insert(macros.begin(), { "VOXELIZE", "1" });
	auto effect = getRenderer()->createEffect("hlsl/gbuffer.fx", macros.data());

	mEffect[mat->getShaderID()] = effect;
	return effect;
}

void Voxelize::voxelize()
{
	auto scene = getScene();
	auto renderer = getRenderer();
	auto context = renderer->getContext();

	renderer->setRasterizer(mRasterizer);
	renderer->setDefaultBlendState();
	renderer->setDefaultDepthStencilState();


	// [use effect framework to bind views, otherwise it would be fail to bind views using OMSetRenderTargetsAndUnorderedAccessViews!!!!]
	//context->OMSetRenderTargetsAndUnorderedAccessViews(1, rtv, 0, 3, 3, uavs, 0);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	auto aabb = scene->getRoot()->getWorldAABB();
	auto vec = aabb.second - aabb.first;

	// make scene smaller than texture3d to ensure the scene cannot be clipped by texture3d
	float bias = 0.00001f;
	float length = std::max(vec.x, std::max(vec.y, vec.z))  + bias;
	renderer->setViewport({0,0, (float)mSize, (float)mSize, 0,1.0f });
	auto center = (aabb.first + aabb.second) * 0.5f;
	float half = length * 0.5f ;

	Matrix proj = DirectX::XMMatrixOrthographicOffCenterLH(-half , half, -half , half , -half , half );
	auto cam = getCamera();
	int vp = 0;

	std::vector<Matrix> views = {
		DirectX::XMMatrixLookToLH(center, Vector3::UnitX, Vector3::UnitY),
		DirectX::XMMatrixLookToLH(center, Vector3::UnitY, Vector3::UnitZ),
		DirectX::XMMatrixLookToLH(center, Vector3::UnitZ, Vector3::UnitY),
	};
	std::array<UINT, 4> c = {0,0,0,0 };
	renderer->clearUnorderedAccess(getUnorderedAccess("voxelalbedo"), c);
	renderer->clearUnorderedAccess(getUnorderedAccess("voxelnormal"), c);
	renderer->clearUnorderedAccess(getUnorderedAccess("voxelmaterial"), c);

	for (auto view : views)
	{
		getScene()->visitRenderables([this, proj, view, vec, vp,cam](const Renderable& r)
		{
			auto effect = getEffect(r.material);
			auto e = effect.lock();

			e->getVariable("albedoRT")->AsUnorderedAccessView()->SetUnorderedAccessView(getUnorderedAccess("voxelalbedo")->getView());
			e->getVariable("normalRT")->AsUnorderedAccessView()->SetUnorderedAccessView(getUnorderedAccess("voxelnormal")->getView());
			e->getVariable("materialRT")->AsUnorderedAccessView()->SetUnorderedAccessView(getUnorderedAccess("voxelmaterial")->getView());

		
			e->getVariable("World")->AsMatrix()->SetMatrix((const float*)&r.tranformation);
			e->getVariable("View")->AsMatrix()->SetMatrix((const float*)&view);
			e->getVariable("Projection")->AsMatrix()->SetMatrix((const float*)&proj);
			e->getVariable("roughness")->AsScalar()->SetFloat(r.material->roughness * getValue<float>("roughness"));
			e->getVariable("metallic")->AsScalar()->SetFloat(r.material->metallic * getValue<float>("metallic"));
			e->getVariable("reflection")->AsScalar()->SetFloat(r.material->reflection);
			e->getVariable("diffuse")->AsVector()->SetFloatVector((const float*)&r.material->diffuse);

			// cannot support height map in voxelization
			e->getVariable("campos")->AsVector()->SetFloatVector((const float*)&Vector3());
			e->getVariable("heightscale")->AsScalar()->SetFloat(getValue<float>("heightscale"));
			e->getVariable("minsamplecount")->AsScalar()->SetInt(getValue<int>("minSampleCount"));
			e->getVariable("maxsamplecount")->AsScalar()->SetInt(getValue<int>("maxSampleCount"));

			e->getVariable("width")->AsScalar()->SetFloat(mSize);
			e->getVariable("height")->AsScalar()->SetFloat(mSize);
			e->getVariable("depth")->AsScalar()->SetFloat(mSize);
			e->getVariable("viewport")->AsScalar()->SetInt(vp);

			auto& texs = r.material->textures;
			std::vector<ID3D11ShaderResourceView*> srvs;
			for (auto&i : texs)
				srvs.push_back(i->Renderer::ShaderResource::getView());
			if (!texs.empty())
				e->getVariable("diffuseTex")->AsShaderResource()->SetResourceArray(srvs.data(), 0, srvs.size());

			getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

			e->render(getRenderer(), [this, &r](ID3DX11EffectPass* pass)
			{
				getRenderer()->setLayout(r.layout.lock()->bind(pass));
				getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
			});

		});

		vp++;
	}


	ID3D11UnorderedAccessView* nil[] = { nullptr,nullptr ,nullptr };
	context->OMSetRenderTargetsAndUnorderedAccessViews(0, 0, 0, 0, 3, nil,0);

}

void Voxelize::voxelLighting()
{
	auto renderer = getRenderer();
	auto scene = getScene();

	auto aabb = scene->getRoot()->getWorldAABB();
	auto vec = aabb.second - aabb.first;
	float length = std::max(vec.x, std::max(vec.y, vec.z));
	float scale = length / mSize;

	auto center = (aabb.first + aabb.second) * 0.5f;
	Vector3 voxelcenter = { mSize / 2.0f, mSize / 2.0f, mSize / 2.0f };
	Vector3 offset = center - voxelcenter * scale;


	ALIGN16
	struct Constants
	{
		Vector3 offset;
		float scaling;
		int size;
		int numpoints;
		int numdirs;
	} c = 
	{
		offset,
		scale,
		mSize,
		getValue<int>("numpoints"),
		getValue<int>("numdirs"),
	};

	auto constants = renderer->createConstantBuffer(sizeof(Constants),&c, sizeof(c));
	auto cs = renderer->createComputeShader("hlsl/voxel_illumination.hlsl","main");


	GPUComputer com(renderer);
	com.setShader(cs);
	com.setConstants({ constants });
	com.setInputs({
		getShaderResource("voxelalbedo"),
		getShaderResource("voxelnormal"),
		getShaderResource("voxelmaterial"),
		getShaderResource("pointlights"),
		getShaderResource("dirlights"),
	});

	com.setOuputs({ getUnorderedAccess("voxelcolor") });
	com.compute(mSize, mSize, mSize);
}

std::unique_ptr<std::vector<char>> Voxelize::readTexture3D(Renderer::Texture3D::Ptr tex)
{
	D3D11_TEXTURE3D_DESC texdesc = tex->getDesc();
	texdesc.MipLevels = 1;
	texdesc.Usage = D3D11_USAGE_STAGING;
	texdesc.BindFlags = 0;
	texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texdesc.MiscFlags = 0;

	auto renderer = getRenderer();
	auto copy = renderer->createTexture3D(texdesc);
	auto context = renderer->getContext();
	context->CopyResource(copy->getTexture(), tex->getTexture());

	int stride = D3D11Helper::sizeof_DXGI_FORMAT(texdesc.Format);
	std::unique_ptr<std::vector<char>> raw(new std::vector<char>(mSize * mSize * mSize * stride));
	char* data = raw->data();
	D3D11_MAPPED_SUBRESOURCE sr;
	HRESULT hr = context->Map(copy->getTexture(), 0, D3D11_MAP_READ, 0, &sr);
	const char* head = (const char*)sr.pData;
	for (int z = 0; z < mSize; ++z)
	{
		for (int y = 0; y < mSize; ++y)
		{
			memcpy(data, head + sr.RowPitch * y + sr.DepthPitch * z, stride * mSize);
			data += mSize * stride;
		}
	}

	context->Unmap(copy->getTexture(), 0);
	renderer->destroyTexture(copy);

	return raw;
}
