#include "ImageProcessing.h"


ImageProcessing::ImageProcessing(Renderer::Ptr r):mRenderer(r), mQuad(r)
{
}

Renderer::TemporaryRT::Ptr ImageProcessing::createOrGet(Renderer::Texture2D::Ptr ptr,float scaling)
{
	auto desc = ptr.lock()->getDesc();
	
	desc.Width = desc.Width * scaling;
	desc.Height = desc.Height* scaling;

	if ((desc.Width == 0 && desc.Height == 0))
		return {};
	if (desc.Width > 8192 || desc.Height > 8192)
		return {};

	desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	if (desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		switch (desc.Format)
		{
		case DXGI_FORMAT_R32_TYPELESS: desc.Format = DXGI_FORMAT_R32_FLOAT; break;
		case DXGI_FORMAT_R24G8_TYPELESS: desc.Format = DXGI_FORMAT_R32_FLOAT; break;
		case DXGI_FORMAT_R16_TYPELESS: desc.Format = DXGI_FORMAT_R16_FLOAT; break;
		case DXGI_FORMAT_R8_TYPELESS: desc.Format = DXGI_FORMAT_R8_UNORM; break;
		default:
			abort();
			break;
		}
		desc.BindFlags &= ~D3D11_BIND_DEPTH_STENCIL;
	}
	return mRenderer->createTemporaryRT(desc);
}

Renderer::TemporaryRT::Ptr ImageProcessing::createOrGet(const D3D11_TEXTURE2D_DESC & texDesc)
{
	auto desc = texDesc;
	desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	return mRenderer->createTemporaryRT(desc);
}


void SamplingBox::init()
{
	mPS = mRenderer->createPixelShader("hlsl/filter.hlsl", "sampleBox");
	mConstants = mRenderer->createBuffer(sizeof(DirectX::SimpleMath::Vector4), D3D11_BIND_CONSTANT_BUFFER);
}

Renderer::TemporaryRT::Ptr SamplingBox::process(Renderer::Texture2D::Ptr tex, float scaling)
{
	auto ret = createOrGet(tex, scaling);

	auto desc = tex.lock()->getDesc();
	DirectX::SimpleMath::Vector4 constants = { 1.0f / (float)desc.Width, 1.0f / (float)desc.Height, mScale[0], mScale[1] };
	mConstants.lock()->blit(constants);
	mQuad.setConstant(mConstants);

	desc = (*ret)->getDesc();
	mQuad.setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad.setTextures({ tex });
	mQuad.setRenderTarget(ret->get());
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultSampler();
	mQuad.setPixelShader(mPS);
	mQuad.draw();

	return ret;
}

void Gaussian::init()
{
	mPS[0] = mRenderer->createPixelShader("hlsl/gaussianblur.hlsl", "main");
	D3D10_SHADER_MACRO macros[] = { { "HORIZONTAL", "1"}, {NULL,NULL} };
	mPS[1] = mRenderer->createPixelShader("hlsl/gaussianblur.hlsl", "main", macros);

}

Renderer::TemporaryRT::Ptr Gaussian::process(Renderer::Texture2D::Ptr tex, float scaling)
{
	auto ret = createOrGet(tex, scaling);

	auto desc = (*ret)->getDesc();
	mQuad.setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultSampler();

	mQuad.setTextures({ tex });
	mQuad.setRenderTarget(ret->get());
	mQuad.setPixelShader(mPS[0]);
	mQuad.draw();

	mQuad.setTextures({ ret->get() });
	ret = createOrGet(ret->get(), scaling);
	mQuad.setRenderTarget(ret->get());
	mQuad.setPixelShader(mPS[1]);
	mQuad.draw();

	return ret;
}

void CubeMapProcessing::init(bool cube)
{
	mIsCubemap = cube;
	D3D10_SHADER_MACRO macros[] = { { "EQUIRECT", "0"}, {NULL,NULL} };
	if (!cube)
		macros[0].Definition = "1";
	mEffect = mRenderer->createEffect("hlsl/cubemap.fx",macros);

	D3D11_INPUT_ELEMENT_DESC modelLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	mLayout = mRenderer->createLayout(modelLayout, ARRAYSIZE(modelLayout));

	{
		Parameters params;
		params["geom"] = "sphere";
		params["size"] = "2";
		params["raidus"] = "1";
		params["resolution"] = "300";
		mCube = GeometryMesh::Ptr(new GeometryMesh(params, mRenderer));
	}
}

Renderer::TemporaryRT::Ptr CubeMapProcessing::process(Renderer::Texture2D::Ptr tex, float scaling)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	auto desc = tex.lock()->getDesc();
	if (!mIsCubemap)
	{
		desc.ArraySize = 6;
		desc.MipLevels = 1;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	}
	desc.Width = 32;
	desc.Height = 32;
	desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	auto ret = createOrGet(desc);

	auto e = mEffect.lock();
	mRenderer->setDefaultBlendState();
	mRenderer->setDefaultBlendState();
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	mRenderer->setRasterizer(rasterDesc);


	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();
	world->SetMatrix((const float*)&Matrix::Identity);
	Matrix op = DirectX::XMMatrixPerspectiveFovLH(3.14159265358f * 0.5f, 1, 0.1f,2.0f);
	proj->SetMatrix((const float*)&op);
	
	mRenderer->setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });

	auto rend = mCube->getMesh(0);
	mRenderer->setIndexBuffer(rend.indices, DXGI_FORMAT_R32_UINT, 0);
	mRenderer->setVertexBuffer(rend.vertices, rend.layout.lock()->getSize(), 0);

	Matrix viewMats[6] = {
		XMMatrixLookAtLH(Vector3::Zero,Vector3::UnitX, Vector3::UnitY),
		XMMatrixLookAtLH(Vector3::Zero,-Vector3::UnitX, Vector3::UnitY),
		XMMatrixLookAtLH(Vector3::Zero,Vector3::UnitY, -Vector3::UnitZ),
		XMMatrixLookAtLH(Vector3::Zero,-Vector3::UnitY, Vector3::UnitZ),
		XMMatrixLookAtLH(Vector3::Zero,Vector3::UnitZ, Vector3::UnitY),
		XMMatrixLookAtLH(Vector3::Zero,-Vector3::UnitZ, Vector3::UnitY),
	};

	for (auto i = 0U; i < 6; ++i)
	{
		auto rt = (*ret)->Renderer::RenderTarget::getView(i);
		mRenderer->setRenderTarget(rt, {});
		view->SetMatrix((const float*)&viewMats[i]);

		e->render(mRenderer, [this, tex,rend,view, viewMats](ID3DX11EffectPass* pass)
		{
			mRenderer->setTexture(tex);
			mRenderer->setLayout(mLayout.lock()->bind(pass));
			mRenderer->getContext()->DrawIndexed(rend.numIndices, 0, 0);
		});


	}

	return ret;
}

void IrradianceCubemap::init(bool cube)
{
	CubeMapProcessing::init(cube);
	mEffect.lock()->setTech("irradiance");
}

void PrefilterCubemap::init(bool iscube)
{
	CubeMapProcessing::init(iscube);
	mEffect.lock()->setTech("prefilter");
}

Renderer::TemporaryRT::Ptr PrefilterCubemap::process(Renderer::Texture2D::Ptr tex, float scaling)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	auto desc = tex->getDesc();

	if (!mIsCubemap)
	{

		desc.ArraySize = 6;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	}
	desc.Width = 512;
	desc.Height = 512;
	desc.MipLevels = 5;
	desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	auto ret = createOrGet(desc);

	auto e = mEffect.lock();
	mRenderer->setDefaultBlendState();
	mRenderer->setDefaultBlendState();
	mRenderer->setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	mRenderer->setRasterizer(rasterDesc);

	auto world = e->getVariable("World")->AsMatrix();
	auto view = e->getVariable("View")->AsMatrix();
	auto proj = e->getVariable("Projection")->AsMatrix();
	world->SetMatrix((const float*)&Matrix::Identity);
	//Matrix op = DirectX::XMMatrixOrthographicLH(2, 2, 0, 1.0f);
	Matrix op = DirectX::XMMatrixPerspectiveFovLH(3.14159265358f * 0.5f, 1, 0.1f, 2.0f);
	//view->SetMatrix((const float*)&cam->getViewMatrix());
	proj->SetMatrix((const float*)&op);


	auto rend = mCube->getMesh(0);
	mRenderer->setIndexBuffer(rend.indices, DXGI_FORMAT_R32_UINT, 0);
	mRenderer->setVertexBuffer(rend.vertices, rend.layout.lock()->getSize(), 0);

	Matrix viewMats[6] = {
		XMMatrixLookAtLH(Vector3::Zero,Vector3::UnitX, Vector3::UnitY),
		XMMatrixLookAtLH(Vector3::Zero,-Vector3::UnitX, Vector3::UnitY),
		XMMatrixLookAtLH(Vector3::Zero,Vector3::UnitY, -Vector3::UnitZ),
		XMMatrixLookAtLH(Vector3::Zero,-Vector3::UnitY, Vector3::UnitZ),
		XMMatrixLookAtLH(Vector3::Zero,Vector3::UnitZ, Vector3::UnitY),
		XMMatrixLookAtLH(Vector3::Zero,-Vector3::UnitZ, Vector3::UnitY),
	};

	int w = desc.Width;
	int h = desc.Height;

	float rs[] = { 0.0f, 0.25f,0.5f,0.75f,1.0f };
	for (auto r = 0U; r < desc.MipLevels; ++r)
	{
		mRenderer->setViewport({ 0.0f, 0.0f, (float)w, (float)h, 0.0f, 0.1f });
		w /= 2;
		h /= 2;

		auto roughness = e->getVariable("roughnessCb")->AsScalar();
		roughness->SetFloat(rs[r]);
		for (auto i = 0U; i < 6; ++i)
		{
			auto rt = (*ret)->Renderer::RenderTarget::getView(i + r * 6);
			mRenderer->setRenderTarget(rt, {});
			view->SetMatrix((const float*)&viewMats[i]);

			e->render(mRenderer, [this,  tex, rend, view, viewMats](ID3DX11EffectPass* pass)
			{
				mRenderer->setTexture(tex);
				mRenderer->setLayout(mLayout.lock()->bind(pass));
				mRenderer->getContext()->DrawIndexed(rend.numIndices, 0, 0);
			});


		}
	}


	return ret;
}

void TileMaxFilter::init(bool normalization, float radius)
{
	mRadius = radius;
	std::string entry = "frag_TileMax2";
	if (normalization)
		entry = "frag_TileMax1";
	mPS = mRenderer->createPixelShader("hlsl/tilemaxfilter.hlsl", entry);
	mConstants = mRenderer->createConstantBuffer(sizeof(Constants));
}

void TileMaxFilter::init(const DirectX::SimpleMath::Vector2 offset, int loop)
{
	mLoop = loop;
	mOffset = offset;
	mPS = mRenderer->createPixelShader("hlsl/tilemaxfilter.hlsl", "frag_TileMaxV");
	mConstants = mRenderer->createConstantBuffer(sizeof(Constants));
}

Renderer::TemporaryRT::Ptr TileMaxFilter::process(Renderer::Texture2D::Ptr tex, float scaling)
{
	//auto ret = createOrGet(tex, scaling);
	auto desc = tex->getDesc();
	desc.Format = DXGI_FORMAT_R16G16_FLOAT;
	desc.Width = desc.Width * scaling;
	desc.Height = desc.Height * scaling;
	auto ret = mRenderer->createTemporaryRT(desc);
	

	Constants c;
	c.texelsize = {1.0f/(float) desc.Width, 1.0f/(float)desc.Height };
	c.offset =  mOffset;
	c.maxradius = mRadius;
	c.loop = mLoop;
	mConstants.lock()->blit(c);
	mQuad.setConstant(mConstants);

	desc = (*ret)->getDesc();
	mQuad.setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad.setTextures({ tex });
	mQuad.setRenderTarget(ret->get());
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultSampler();
	mQuad.setPixelShader(mPS);
	mQuad.draw();

	return ret;
}

void NeighborMaxFilter::init()
{
	mPS = mRenderer->createPixelShader("hlsl/tilemaxfilter.hlsl", "frag_NeighborMax");
	mConstants = mRenderer->createConstantBuffer(sizeof(Constants));
}

Renderer::TemporaryRT::Ptr NeighborMaxFilter::process(Renderer::Texture2D::Ptr tex, float scaling)
{
	//auto ret = createOrGet(tex, scaling);


	//auto desc = tex.lock()->getDesc();

	auto desc = tex->getDesc();
	desc.Format = DXGI_FORMAT_R16G16_FLOAT;
	desc.Width = desc.Width * scaling;
	desc.Height = desc.Height * scaling;
	auto ret = mRenderer->createTemporaryRT(desc);

	Constants c;
	c.texelsize = { 1.0f/(float)desc.Width, 1.0f/(float)desc.Height };
	mConstants.lock()->blit(c);
	mQuad.setConstant(mConstants);

	desc = (*ret)->getDesc();
	mQuad.setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	mQuad.setTextures({ tex });
	mQuad.setRenderTarget(ret->get());
	mQuad.setDefaultBlend(false);
	mQuad.setDefaultSampler();
	mQuad.setPixelShader(mPS);
	mQuad.draw();

	return ret;
}
