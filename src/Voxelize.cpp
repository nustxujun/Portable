#include "Voxelize.h"



void Voxelize::init(int size)
{
	mName = "Voxelize";
	mSize = 1;
	while (mSize < size)
		mSize *= 8;
	
	auto renderer = getRenderer();

	D3D11_TEXTURE3D_DESC texdesc;
	texdesc.Width = mSize;
	texdesc.Height = mSize;
	texdesc.Depth = mSize;
	texdesc.MipLevels = 1;
	texdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texdesc.CPUAccessFlags = 0;
	texdesc.MiscFlags = 0;
	auto tex = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelalbedo", tex);

	texdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	tex = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelnormal", tex);

	texdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	tex = renderer->createTexture3D(texdesc);
	addUnorderedAccess("voxelmaterial", tex);


	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	mRasterizer = renderer->createOrGetRasterizer(rasterDesc);
}

void Voxelize::render(Renderer::Texture2D::Ptr rt)
{
	auto scene = getScene();
	auto renderer = getRenderer();
	auto context = renderer->getContext();

	renderer->setRasterizer(mRasterizer);

	std::vector<ID3D11UnorderedAccessView*> uavs = {
		getUnorderedAccess("voxelalbedo")->getView(),
		getUnorderedAccess("voxelnormal")->getView(),
		getUnorderedAccess("voxelmaterial")->getView(),
	};

	context->OMSetRenderTargetsAndUnorderedAccessViews(0, 0, 0, 0, 3, uavs.data(), 0);
	renderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::vector<Matrix> views = {
		Matrix::CreateLookAt(Vector3::Zero, Vector3::UnitX, Vector3::UnitY),
		Matrix::CreateLookAt(Vector3::Zero, Vector3::UnitY, Vector3::UnitZ),
		Matrix::CreateLookAt(Vector3::Zero, Vector3::UnitZ, Vector3::UnitY),
	};

	auto aabb = scene->getRoot()->getWorldAABB();
	auto vec = aabb.second - aabb.first;
	float length = std::max(vec.x, std::max(vec.y, vec.z));
	renderer->setViewport({ (float)mSize, (float)mSize, 0,1.0f,0,0, });
	auto center = (aabb.first + aabb.second) * 0.5f;
	float half = length * 0.5f;
	Matrix proj = DirectX::XMMatrixOrthographicOffCenterLH(-half + center.x, half + center.x, -half + center.y, half + center.y, -half + center.z, half + center.z);

	int vp = 0;
	for (auto view: views)
	{
		getScene()->visitRenderables([this, proj,view, vec, vp](const Renderable& r)
		{
			auto effect = getEffect(r.material);
			auto e = effect.lock();
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

			e->getVariable("width")->AsScalar()->SetFloat(vec.x);
			e->getVariable("height")->AsScalar()->SetFloat(vec.y);
			e->getVariable("depth")->AsScalar()->SetFloat(vec.z);
			e->getVariable("viewport")->AsScalar()->SetInt(vp);

			getRenderer()->setIndexBuffer(r.indices, DXGI_FORMAT_R32_UINT, 0);
			getRenderer()->setVertexBuffer(r.vertices, r.layout.lock()->getSize(), 0);

			e->render(getRenderer(), [this, &r, e](ID3DX11EffectPass* pass)
			{
				getRenderer()->setTextures(r.material->textures);
				getRenderer()->setLayout(r.layout.lock()->bind(pass));
				getRenderer()->getContext()->DrawIndexed(r.numIndices, 0, 0);
			});

		});

		vp++;
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
