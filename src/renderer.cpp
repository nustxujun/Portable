#include "renderer.h"
#include <array>
#include <D3Dcompiler.h>
#include <fstream>
#include "D3D11Helper.h"
#include <regex>
#include <iostream>
#include <sstream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define USE_PROFILE
#define RELEASE_WITH_DEBUGINFO

void Renderer::checkResult(HRESULT hr)
{
	if (hr == S_OK) return;
	TCHAR msg[MAX_PATH] = {0};
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, hr, 0, msg, sizeof(msg), 0);
	MessageBox(NULL, msg, NULL, MB_ICONERROR);
	abort();
}

void Renderer::error(const std::string& err)
{
	MessageBoxA(NULL, err.c_str(), NULL, MB_ICONERROR);
	abort();
}


void Renderer::initCompiledShaders()
{
	CreateDirectoryA("cache/", NULL);
	WIN32_FIND_DATAA data;
	auto h = FindFirstFileA("cache/*", &data);
	
	std::vector<std::string> files;
	while (h != INVALID_HANDLE_VALUE)
	{
		if (data.cFileName[0] != '.')
			files.push_back( data.cFileName);
		if (!FindNextFileA(h, &data))
			break;
	}
	FindClose(h);


	std::string prefix = "cache/";
	for (auto&i : files)
	{
		std::fstream f(prefix + i, std::ios::in | std::ios::binary);
		if (!f)
			continue;
		unsigned int size = 0;
		f.read((char*)&size, sizeof(size));
		size_t h;
		f.read((char*)&h, sizeof(h));
		std::vector<char> buffer(size);
		f.read(buffer.data(), size);
		std::stringstream ss;
		ss << i;
		size_t hash;
		ss >> std::hex>> hash;
		mCompiledShader[hash] = SharedCompiledData(new CompiledData(h, buffer ));
	}
}

void Renderer::init(HWND win, int width, int height)
{
	initCompiledShaders();
	mWidth = width;
	mHeight = height;
	D3D_FEATURE_LEVEL featureLevels[] =
	{
#ifdef USING_D3D11_3
		D3D_FEATURE_LEVEL_12_0,
#else
		D3D_FEATURE_LEVEL_11_0,
#endif
		//D3D_FEATURE_LEVEL_10_1,
		//D3D_FEATURE_LEVEL_10_0,
	};

	DXGI_SWAP_CHAIN_DESC sd = 
	{
		{
			width, 
			height,
			{60, 1},
			DXGI_FORMAT_R8G8B8A8_UNORM, 
			DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			DXGI_MODE_SCALING_UNSPECIFIED,
		},
		{1, 0 },
		DXGI_USAGE_RENDER_TARGET_OUTPUT,
		1,
		win,
		TRUE,
		DXGI_SWAP_EFFECT_DISCARD,
		0,
	};

	UINT flags = 0;
#if (defined _DEBUG ) || (defined RELEASE_WITH_DEBUGINFO)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	checkResult(D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		flags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&sd,
		&mSwapChain,
		&mDevice,
		&mFeatureLevel,
		&mContext)
	);

	

	ID3D11Texture2D* backbuffer = NULL;
	checkResult( mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer) );

	auto shared = std::shared_ptr<Texture2D>(new Texture2D(this, backbuffer));
	mTextures.emplace_back(shared);
	mBackbuffer = shared;

	mDefaultDepthStencil = createDepthStencil(width, height, DXGI_FORMAT_D24_UNORM_S8_UINT);

	D3D11_DEPTH_STENCIL_DESC dsdesc =
	{
		TRUE,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS_EQUAL,
		FALSE,
		D3D11_DEFAULT_STENCIL_READ_MASK,
		D3D11_DEFAULT_STENCIL_WRITE_MASK,
		{
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_KEEP,
			D3D11_COMPARISON_ALWAYS,
		},
		{
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_KEEP,
			D3D11_STENCIL_OP_KEEP,
			D3D11_COMPARISON_ALWAYS
		}
	};
	createDepthStencilState("depth_write_less_equal", dsdesc);


	D3D11_QUERY_DESC d;
	d.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	d.MiscFlags = 0;

	mDevice->CreateQuery(&d, &mDisjoint);

#ifdef USE_PROFILE
	mContext->Begin(mDisjoint);
#endif
}


void Renderer::present()
{
	HRESULT hr = mSwapChain->Present(0, DXGI_PRESENT_TEST);

	switch (hr)
	{
	case S_OK: mSwapChain->Present(0, 0); break;
	case DXGI_STATUS_OCCLUDED: break;
	default:
		checkResult(hr);
		break;
	}
#ifdef USE_PROFILE

	mContext->End(mDisjoint);
	while (mContext->GetData(mDisjoint, NULL, 0, 0) == S_FALSE);

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
	mContext->GetData(mDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0);
	if (tsDisjoint.Disjoint != TRUE)
	{
		for (auto& i : mProfiles)
			i->getData(tsDisjoint.Frequency);
	}

	mContext->Begin(mDisjoint);
#endif
}

void Renderer::clearDepth(DepthStencil::Ptr rt, float depth)
{
	mContext->ClearDepthStencilView(rt->getView(), D3D11_CLEAR_DEPTH , depth, 0);
}

void Renderer::clearStencil(DepthStencil::Ptr rt, UINT stencil)
{
	mContext->ClearDepthStencilView(rt->getView(), D3D11_CLEAR_STENCIL, 0, stencil);
}

void Renderer::clearDepthStencil(DepthStencil::Ptr rt, float depth, UINT stencil)
{
	mContext->ClearDepthStencilView(rt->getView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

void Renderer::clearRenderTarget(RenderTarget::Ptr rt, const std::array<float, 4>& color, size_t index )
{
	if (index == -1)
	{
		for (size_t i = 0; i < rt->getNumViews(); ++i)
			mContext->ClearRenderTargetView(rt->getView(i), color.data());
	}
	else
		mContext->ClearRenderTargetView(rt->getView(index), color.data());
}

void Renderer::clearUnorderedAccess(UnorderedAccess::Ptr rt, const std::array<float, 4>& value, size_t index)
{
	if (index == -1)
	{
		for (size_t i = 0; i < rt->getNumViews(); ++i)
			mContext->ClearUnorderedAccessViewFloat(rt->getView(i), value.data());
	}
	else
		mContext->ClearUnorderedAccessViewFloat(rt->getView(index), value.data());
}

void Renderer::clearUnorderedAccess(UnorderedAccess::Ptr rt, const std::array<UINT, 4>& value, size_t index)
{
	if (index == -1)
	{
		for (size_t i = 0; i < rt->getNumViews(); ++i)
			mContext->ClearUnorderedAccessViewUint(rt->getView(i), value.data());
	}
	else
		mContext->ClearUnorderedAccessViewUint(rt->getView(index), value.data());
}


void Renderer::setTexture(ShaderResource::Ptr srv)
{
	removeShaderResourceViews();
	mSRVState = 1;
	auto ptr = srv.lock();
	if (ptr == nullptr) return;
	auto in = ptr->getView();
	mContext->PSSetShaderResources(0, 1, &in);
}

void Renderer::setTextures(const std::vector<ShaderResource::Ptr>& srvs)
{
	removeShaderResourceViews();
	
	std::vector<ID3D11ShaderResourceView*> list;
	list.reserve(srvs.size());
	for (auto&i : srvs)
	{
		if (i.lock())
			list.push_back(*i.lock());
		else
			list.push_back(nullptr);
	}
	
	mSRVState = list.size();

	mContext->PSSetShaderResources(0, list.size(), list.data());
}

void Renderer::removeShaderResourceViews()
{
	if (mSRVState == 0)
		return;
	std::vector<ID3D11ShaderResourceView*> nil(mSRVState);
	mContext->PSSetShaderResources(0, nil.size(), nil.data());
	mSRVState = 0;
}

void Renderer::setSampler(Sampler::Ptr s)
{
	removeSamplers();
	mSamplersState = 1;
	auto ptr = s.lock();
	if (ptr != NULL)
	{
		ID3D11SamplerState* ss = *ptr;
		mContext->PSSetSamplers(0, 1, &ss);
	}
}

void Renderer::setSamplers(const std::vector<Sampler::Ptr>& ss)
{
	removeSamplers();
	std::vector<ID3D11SamplerState*> list;
	list.reserve(ss.size());

	for (auto& i : ss)
	{
		list.push_back(*i.lock());
	}
	mSamplersState = list.size();
	mContext->PSSetSamplers(0, list.size(), list.data());


}

void Renderer::removeSamplers()
{
	ID3D11SamplerState* nil = 0;
	for (int i = 0; i < mSamplersState; ++i)
		mContext->PSSetSamplers(i, 1, &nil);
	mSamplersState = 0;
}

Renderer::Sampler::Ptr Renderer::getSampler(const std::string & name)
{
	auto ret = mSamplers.find(name);
	if (ret == mSamplers.end())
		return Sampler::Ptr();
	else
		return ret->second;
}

void Renderer::setRenderTarget(RenderTarget::Ptr rt, DepthStencil::Ptr ds)
{
	auto ptr = rt.lock();
	ID3D11RenderTargetView* rtv = nullptr;
	if (ptr != nullptr)
	{
		rtv = *ptr;
	}
	auto dsvptr = ds.lock();
	ID3D11DepthStencilView* dsv = nullptr;
	if (dsvptr != nullptr)
		dsv = *dsvptr;
	mContext->OMSetRenderTargets(1, &rtv, dsv);
}

void Renderer::setRenderTarget(ID3D11RenderTargetView* rt, DepthStencil::Ptr ds )
{
	auto dsvptr = ds.lock();
	ID3D11DepthStencilView* dsv = nullptr;
	if (dsvptr != nullptr)
		dsv = *dsvptr;
	mContext->OMSetRenderTargets(1, &rt, dsv);
}

void Renderer::setRenderTargets(const std::vector<RenderTarget::Ptr>& rts, DepthStencil::Ptr ds)
{
	ID3D11DepthStencilView* dsv = 0;
	if (!ds.expired())
		dsv = *ds.lock();

	std::vector<ID3D11RenderTargetView*> list;
	list.reserve(rts.size());

	for (auto i : rts)
		list.push_back(*i.lock());
	
	mContext->OMSetRenderTargets(list.size(), list.data(), dsv);
}

void Renderer::setRenderTargets(ID3D11RenderTargetView** rts, size_t size, DepthStencil::Ptr ds )
{
	ID3D11DepthStencilView* dsv = 0;
	if (!ds.expired())
		dsv = *ds.lock();

	mContext->OMSetRenderTargets(size, rts, dsv);
}


void Renderer::removeRenderTargets()
{
	ID3D11RenderTargetView* clears[8] = { 0 };
	mContext->OMSetRenderTargets(8, clears, 0);
}


void Renderer::setVSConstantBuffers(const std::vector<Buffer::Ptr>& bs)
{
	setConstantBuffersImpl(bs, [this](size_t num, ID3D11Buffer** buffers) {
		mContext->VSSetConstantBuffers(0, num, buffers);
	});
}

void Renderer::setPSConstantBuffers(const std::vector<Buffer::Ptr>& bs)
{
	setConstantBuffersImpl(bs, [this](size_t num, ID3D11Buffer** buffers) {
		mContext->PSSetConstantBuffers(0, num, buffers);
	});
}


void Renderer::setConstantBuffersImpl(const std::vector<Buffer::Ptr>& bs, std::function<void(size_t, ID3D11Buffer**)> f)
{
	std::vector<ID3D11Buffer*> list;
	list.reserve(bs.size());

	for (auto i : bs)
		list.push_back(*(i.lock()));

	f(list.size(), list.data());
}


//void Renderer::removeConstantBuffers()
//{
//	ID3D11Buffer* nil = 0;
//	for (size_t i = 0; i < mConstantState; ++i)
//		mContext->PSSetConstantBuffers(i, 1, &nil);
//	mConstantState = 0;
//}

void Renderer::setRasterizer(Rasterizer::Ptr r)
{
	auto ptr = r.lock();
	if (ptr == nullptr)
		return;

	mContext->RSSetState(*ptr);
}

void Renderer::setRasterizer(const D3D11_RASTERIZER_DESC& desc)
{
	setRasterizer(createOrGetRasterizer(desc));
}

void Renderer::setDefaultRasterizer()
{
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	auto dft = createOrGetRasterizer(rasterDesc);
	setRasterizer(dft);
}

void Renderer::setLayout(ID3D11InputLayout*  layout)
{
	mContext->IASetInputLayout(layout);
}

void Renderer::setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY pri)
{
	mContext->IASetPrimitiveTopology(pri);
}

void Renderer::setIndexBuffer(Buffer::Ptr b, DXGI_FORMAT format, int offset)
{
	auto ptr = b.lock();
	if (ptr == nullptr) return;
	mContext->IASetIndexBuffer(*ptr, format, offset);
}

void Renderer::setVertexBuffer(Buffer::Ptr b, UINT stride, UINT offset)
{
	auto ptr = b.lock();
	if (ptr == nullptr) return;

	ID3D11Buffer* vb = *ptr;
	mContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
}

void Renderer::setVertexBuffers(const std::vector<Buffer::Ptr>& b, const UINT * stride, const UINT * offset)
{
	std::vector<ID3D11Buffer*> list;
	list.reserve(b.size());

	for (auto i : b)
		list.push_back(*i.lock());

	mContext->IASetVertexBuffers(0, b.size(), list.data(), stride, offset);
}

void Renderer::setViewport(const D3D11_VIEWPORT & vp)
{
	mContext->RSSetViewports(1, &vp);
}


void Renderer::setDepthStencilState(const std::string & name, UINT stencil)
{
	auto ret = mDepthStencilStates.find(name);
	if (ret == mDepthStencilStates.end())
		return error(Common::format("cannot find depthstencilstate ", name));

	mContext->OMSetDepthStencilState(*ret->second, stencil);
}

void Renderer::setDepthStencilState(DepthStencilState::Weak dss, UINT stencil)
{
	mContext->OMSetDepthStencilState(*dss.lock(), stencil);
}

//void Renderer::setBlendState(const D3D11_BLEND_DESC& desc, const std::array<float, 4>& factor , size_t mask )
//{
//	std::string name((char*)&desc, sizeof(desc));
//	auto ret = mBlendStates.find(name);
//	if (ret == mBlendStates.end())
//	{
//		ID3D11BlendState* state;
//		checkResult(mDevice->CreateBlendState(&desc, &state));
//		auto newstate = mBlendStates.emplace(name, new BlendState(state));
//		ret = newstate.first;
//	}
//
//	mContext->OMSetBlendState(*ret->second, factor.data(), mask);
//}

void Renderer::setBlendState(const std::string& name, const std::array<float, 4>& factor , size_t mask )
{
	auto ret = mBlendStates.find(name);
	if (ret == mBlendStates.end())
		return error(Common::format("cannot find blend state:", name ));

	mContext->OMSetBlendState(*ret->second, factor.data(), mask);
}

void Renderer::setBlendState(BlendState::Weak blend, const std::array<float, 4>& factor, size_t mask)
{
	mContext->OMSetBlendState(*blend.lock(), factor.data(), mask);
}

void Renderer::setDefaultBlendState()
{
	float factor[4] = { 1,1,1,1 };
	size_t mask = 0xffffffff;
	mContext->OMSetBlendState(NULL, factor, mask);
}

void Renderer::setDefaultDepthStencilState()
{
	mContext->OMSetDepthStencilState(NULL, 0);
}

void Renderer::setVertexShader(VertexShader::Weak shader)
{
	auto ptr = shader.lock();
	ID3D11VertexShader* vs = nullptr;
	if (ptr)
		vs = *ptr;
	mContext->VSSetShader(vs, NULL, 0);
}

void Renderer::setPixelShader(PixelShader::Weak shader)
{
	auto ptr = shader.lock();
	ID3D11PixelShader* ps = nullptr;
	if (ptr)
		ps = *ptr;
	mContext->PSSetShader(ps, NULL, 0);
}

void Renderer::setComputeShder(ComputeShader::Weak shader)
{
	auto ptr = shader.lock();
	ID3D11ComputeShader* cs = nullptr;
	if (ptr)
		cs = *ptr;
	mContext->CSSetShader(cs, NULL,0);
}

void Renderer::uninit()
{
	mProfiles.clear();
	mDisjoint->Release();
	mBlendStates.clear();
	mDepthStencilStates.clear();
	mRasterizers.clear();
	mFonts.clear();
	mVSs.clear();
	mPSs.clear();
	mCSs.clear();
	mSamplers.clear();
	mTextures.clear();
	mEffects.clear();
	mLayouts.clear();
	mBuffers.clear();
	mSwapChain->Release();
	mContext->Release();
	if (mDevice->Release() != 0)
	{
		ID3D11Debug *d3dDebug;
		HRESULT hr = mDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug));
		if (SUCCEEDED(hr))
		{
			hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
		}
		if (d3dDebug)
			d3dDebug->Release();


		MessageBox(NULL, TEXT("some objects were not released."), NULL, NULL);
	}

}

Renderer::BlendState::Weak Renderer::createBlendState(const std::string & name, const D3D11_BLEND_DESC & desc)
{
	auto ret = mBlendStates.find(name);
	if (ret != mBlendStates.end())
		return ret->second;

	ID3D11BlendState* state;
	checkResult(mDevice->CreateBlendState(&desc, &state));
	auto newstate = mBlendStates.emplace(name, new BlendState(state));
	ret = newstate.first;
	return ret->second;
}

Renderer::Sampler::Ptr Renderer::createSampler(const std::string& name, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV, D3D11_TEXTURE_ADDRESS_MODE addrW, D3D11_COMPARISON_FUNC cmpfunc, float minlod, float maxlod)
{
	D3D11_SAMPLER_DESC sampPointDesc;
	ZeroMemory(&sampPointDesc, sizeof(sampPointDesc));
	sampPointDesc.Filter = filter;
	sampPointDesc.AddressU = addrU;
	sampPointDesc.AddressV = addrV;
	sampPointDesc.AddressW = addrW;
	sampPointDesc.ComparisonFunc = cmpfunc;
	sampPointDesc.MinLOD = minlod;
	sampPointDesc.MaxLOD = maxlod;
	ID3D11SamplerState* sampler;
	checkResult(mDevice->CreateSamplerState(&sampPointDesc, &sampler));
	auto ret = mSamplers.emplace(name, new Sampler(sampler));
	return Sampler::Ptr(ret.first->second);
}

Renderer::Texture2D::Ptr Renderer::createTexture(const std::string & filename, UINT miplevels )
{
	auto ret = mTextureMap.find(filename);
	if (ret != mTextureMap.end())
		return ret->second;


	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	void *data = 0;
	int width, height, nrComponents;


	if ( stbi_is_hdr(filename.c_str()))
	{
		format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		data = stbi_loadf(filename.c_str(), &width, &height, &nrComponents, 4);
	}
	else
		data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4);

	if (data)
	{
		D3D11_TEXTURE2D_DESC desc = { 0 };
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = miplevels;
		desc.Format = format;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		desc.ArraySize = 1;
		desc.SampleDesc.Quality = 0;
		desc.SampleDesc.Count = 1;

		auto tex = createTexture(desc);

		if (data)
		{
			tex->blit(data,0);
			mContext->GenerateMips(tex->ShaderResource::getView());
		}

		stbi_image_free(data);
		return tex;
	}
	else
	{
		D3DX11_IMAGE_LOAD_INFO info;
		info.MipLevels = miplevels;
		ID3D11Texture2D* tex;
		checkResult(D3DX11CreateTextureFromFileA(mDevice, filename.c_str(), &info, NULL, (ID3D11Resource**)&tex,NULL));
		auto ptr = std::shared_ptr<Texture2D>(new Texture2D(this, tex));
		mTextures.emplace_back(ptr);
		return ptr;
	}



	
}

Renderer::Texture2D::Ptr Renderer::createTexture( const D3D11_TEXTURE2D_DESC& desc)
{
	//D3D11_SUBRESOURCE_DATA initdata = {0};
	//initdata.pSysMem = data;
	//initdata.SysMemPitch = desc.Width * D3D11Helper::sizeof_DXGI_FORMAT(desc.Format);
	ID3D11Texture2D* tex;
	//checkResult(mDevice->CreateTexture2D(&desc, data == nullptr ? nullptr : &initdata, &tex));
	checkResult(mDevice->CreateTexture2D(&desc, NULL, &tex));
	auto ptr = std::shared_ptr<Texture2D>(new Texture2D(this, tex));
	mTextures.emplace_back(ptr);

	return ptr;
}

Renderer::Texture3D::Ptr Renderer::createTexture3D(const D3D11_TEXTURE3D_DESC & desc, const void * data, size_t size)
{
	D3D11_SUBRESOURCE_DATA initdata = { 0 };
	initdata.pSysMem = data;
	ID3D11Texture3D* tex;
	checkResult(mDevice->CreateTexture3D(&desc, data == nullptr ? nullptr : &initdata, &tex));

	auto ptr = std::shared_ptr<Texture3D>(new Texture3D(this, tex));
	mTextures.emplace_back(ptr);
	return ptr;
}

Renderer::Texture2D::Ptr Renderer::createTextureCube(const std::string& file)
{
	ID3D11Texture2D* tex;
	D3DX11_IMAGE_LOAD_INFO info;
	info.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	checkResult(D3DX11CreateTextureFromFileA(mDevice, file.c_str(), &info, NULL, (ID3D11Resource**)&tex, NULL));

	auto ptr = std::shared_ptr<Texture2D>(new Texture2D(this, tex));
	mTextures.emplace_back(ptr);
	return ptr;
}

Renderer::Texture2D::Ptr Renderer::createTextureCube(const std::array<std::string, 6>& files)
{
	if (files[1].empty())
		return createTextureCube(files[0]);
	Texture2D::Ptr faces[6];
	for (size_t i = 0; i < 6; ++i)
	{
		faces[i] = createTexture(files[i]);
	}
	D3D11_TEXTURE2D_DESC facedesc = faces[0].lock()->getDesc();
	
	D3D11_TEXTURE2D_DESC texdesc = facedesc;
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 6;
	texdesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* cube;
	checkResult(mDevice->CreateTexture2D(&texdesc, NULL, &cube));

	for (size_t i = 0; i < 6; ++i)
	{
		auto tex = faces[i];
		mContext->CopySubresourceRegion(cube, i,0, 0, 0, tex.lock()->getTexture(), 0, NULL);
		destroyTexture(faces[i]);
	}


	auto ptr = std::shared_ptr<Texture2D>(new Texture2D(this, cube));
	mTextures.emplace_back(ptr);
	return ptr;
}

void Renderer::destroyTexture(Texture::Ptr tex)
{
	mTextures.erase(std::remove(mTextures.begin(), mTextures.end(), tex.lock()));
}



Renderer::Texture2D::Ptr Renderer::createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage)
{
	D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = width;
	desc.Height = height;
	desc.Format = format;
	desc.Usage = usage;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	return createTexture(desc);
}

Renderer::TemporaryRT::Ptr Renderer::createTemporaryRT(const D3D11_TEXTURE2D_DESC & desc)
{
	std::string format = Common::format(
		desc.Width,
		desc.Height,
		desc.MipLevels,
		desc.ArraySize,
		desc.Format,
		desc.SampleDesc.Count,
		desc.SampleDesc.Quality,
		desc.Usage,
		desc.BindFlags,
		desc.CPUAccessFlags,
		desc.MiscFlags
	);


	auto hash = Common::hash(format);
	auto ret = mTemporaryRTs.find(hash);
	if (ret != mTemporaryRTs.end())
	{
		for (auto & t : ret->second)
		{
			if (*t.first == false)
				return TemporaryRT::create(t.first, t.second);
		}
	}

	auto rt = createTexture(desc);
	auto ref = std::shared_ptr<bool>(new bool(false));
	mTemporaryRTs[hash].push_back({ ref, rt });
	return TemporaryRT::create(ref,rt);
}

void Renderer::destroyTemporaryRT(TemporaryRT::Ptr temp)
{
	// only pass unique ptr here and do nothing, then unique ptr will delete temporary rt and release the reference after leaving this function
}

Renderer::Buffer::Ptr Renderer::createBuffer(int size, D3D11_BIND_FLAG flag, const D3D11_SUBRESOURCE_DATA* initialdata, D3D11_USAGE usage, size_t CPUaccess)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = usage;
	bd.ByteWidth = size;
	bd.BindFlags = flag;
	bd.CPUAccessFlags = CPUaccess;

	mBuffers.emplace_back(new Buffer(this, bd, initialdata));
	return mBuffers.back();
}

Renderer::Buffer::Ptr Renderer::createRWBuffer(int size, int stride, DXGI_FORMAT format, size_t bindflag, D3D11_USAGE usage, size_t CPUaccess)
{
	mBuffers.emplace_back(new Buffer(this,size,stride,format,bindflag,usage,CPUaccess));

	return mBuffers.back();
}
#define ALIGN(x,y) (((x + y - 1) & ~(y - 1)) )

Renderer::Buffer::Ptr Renderer::createConstantBuffer(int size, void* data , size_t datasize)
{
	size = ALIGN(size, 16);
	D3D11_SUBRESOURCE_DATA initdata = { 0 };
	initdata.pSysMem = data;
	return createBuffer(size, D3D11_BIND_CONSTANT_BUFFER, data? &initdata:nullptr, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}


Renderer::SharedCompiledData Renderer::compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro)
{

	std::string hashstr = filename + entryPoint + shaderModel;
	auto M = macro;
	while (M && M->Name)
	{
		hashstr += M->Name;
		hashstr += M->Definition;
		M++;
	}

#if (defined _DEBUG)
	hashstr += "_DEBUG";
#endif
#if(defined WIN32)
	hashstr += "WIN32";
#endif

	std::fstream shaderfile(filename, std::ios::in | std::ios::binary);
	if (!shaderfile)
		error(Common::format("cannot find shader file ", filename));
	shaderfile.seekg(0, std::ios::end);
	size_t datasize = shaderfile.tellg();
	shaderfile.seekg(0);
	std::vector<char> mem(datasize);
	shaderfile.read(mem.data(), datasize);
	size_t rawhash = Common::hash(mem.data(), datasize);
	shaderfile.close();
	size_t hash = Common::hash(hashstr);
	auto shader = mCompiledShader.find(hash);
	if (shader != mCompiledShader.end())
	{

		if (rawhash == shader->second->getHash())
			return shader->second;
	}

	int flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if (defined _DEBUG ) || (defined RELEASE_WITH_DEBUGINFO)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
#endif




	ID3D10Blob* blob;
	ID3D10Blob* err;
	std::cout << "compile " << filename << " "<< entryPoint << " " << shaderModel << " ... ";
	auto curtime = GetTickCount();
	HRESULT hr = D3DX11CompileFromMemory(mem.data(), mem.size(),filename.c_str(), macro, NULL, entryPoint.c_str(), shaderModel.c_str(), flags, 0, NULL, &blob, &err, NULL);
	//HRESULT hr = D3DX11CompileFromFileA(filename.c_str(), macro, NULL, entryPoint.c_str(), shaderModel.c_str(), flags, 0, NULL, &blob, &err, NULL);

	std::cout << GetTickCount() - curtime << "ms." << std::endl;
	
	if (hr != S_OK)
	{
		if (err)
		{
			MessageBoxA(NULL, (LPCSTR)err->GetBufferPointer(), NULL, NULL);
			err->Release();
			abort();
		}
		else
			checkResult(hr);
	}
	std::string path = "cache/";
	std::stringstream ss;
	ss << path << std::hex << hash;
	std::fstream f(ss.str(), std::ios::out | std::ios::binary);
	unsigned int s = blob->GetBufferSize();
	f.write((const char*)&s, sizeof(s));
	f.write((const char*)&rawhash, sizeof(rawhash));
	f.write((const char*)blob->GetBufferPointer(), s);
	return SharedCompiledData(new CompiledData(rawhash,blob));
}

Renderer::Effect::Ptr Renderer::createEffect(const std::string& file, const D3D10_SHADER_MACRO* macro)
{
	auto blob = compileFile(file, "", "fx_5_0", macro);
	return createEffect(blob->GetBufferPointer(), blob->GetBufferSize());
}

Renderer::Effect::Ptr Renderer::createEffect(void* data, size_t size)
{
	auto ptr = std::shared_ptr<Effect>(new Effect(this, data, size));
	mEffects.push_back(ptr);
	return ptr;
}

Renderer::VertexShader::Weak Renderer::createVertexShader(const void * data, size_t size)
{
	auto hash = Common::hash(data, size);
	auto ret = mVSs.find(hash);
	if (ret != mVSs.end())
		return ret->second;

	ID3D11VertexShader* vs;
	checkResult(mDevice->CreateVertexShader(data, size, NULL, &vs));
	mVSs[hash] = std::shared_ptr<VertexShader> (new VertexShader(vs, data, size));
	return mVSs[hash];
}

Renderer::VertexShader::Weak Renderer::createVertexShader(const std::string & file, const std::string & entry, const D3D10_SHADER_MACRO * macro)
{
	auto blob = compileFile(file, entry, "vs_5_0", macro);
	return createVertexShader(blob->GetBufferPointer(), blob->GetBufferSize());
}

Renderer::PixelShader::Weak Renderer::createPixelShader(const void * data, size_t size)
{
	auto hash = Common::hash(data, size);
	auto ret = mPSs.find(hash);
	if (ret != mPSs.end())
		return ret->second;

	ID3D11PixelShader* ps;
	checkResult(mDevice->CreatePixelShader(data, size, NULL, &ps));
	mPSs[hash] = std::shared_ptr<PixelShader>(new PixelShader(ps));
	return mPSs[hash];
}

Renderer::PixelShader::Weak Renderer::createPixelShader(const std::string& file, const std::string& entry, const D3D10_SHADER_MACRO* macro)
{
	auto blob = compileFile(file, entry, "ps_5_0", macro);
	return createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());
}

Renderer::ComputeShader::Weak Renderer::createComputeShader(const void * data, size_t size)
{
	auto hash = Common::hash(data, size);
	auto ret = mCSs.find(hash);
	if (ret != mCSs.end())
		return ret->second;


	ID3D11ComputeShader* cs;
	checkResult(mDevice->CreateComputeShader(data, size, NULL, &cs));
	mCSs[hash] = std::shared_ptr<ComputeShader>(new ComputeShader(cs));
	return mCSs[hash];
}

Renderer::ComputeShader::Weak Renderer::createComputeShader(const std::string& name, const std::string& entry)
{
	auto blob = compileFile(name, entry, "cs_5_0");
	return createComputeShader(blob->GetBufferPointer(), blob->GetBufferSize());
}


Renderer::Layout::Ptr Renderer::createLayout(const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count)
{
	mLayouts.emplace_back(new Layout(this, descarray, count));
	return mLayouts.back();
}

Renderer::Font::Ptr Renderer::createOrGetFont(const std::wstring & font)
{
	auto ret = mFonts.find(font);
	if (ret != mFonts.end())
		return ret->second;

	std::shared_ptr<Font> shared (new Font(this,font));
	mFonts.emplace(font, shared);
	return Font::Ptr(shared);
}

Renderer::Rasterizer::Ptr Renderer::createOrGetRasterizer(const D3D11_RASTERIZER_DESC& desc)
{
	std::string params = Common::format(
		desc.FillMode,
		desc.CullMode,
		desc.FrontCounterClockwise,
		desc.DepthBias,
		desc.DepthBiasClamp,
		desc.SlopeScaledDepthBias,
		desc.DepthClipEnable,
		desc.ScissorEnable,
		desc.MultisampleEnable,
		desc.AntialiasedLineEnable
	);
	auto key = Common::hash(params);

	auto ret = mRasterizers.find(key);
	if (ret != mRasterizers.end())
		return ret->second;

	//D3D11_RASTERIZER_DESC rasterDesc;
	//rasterDesc.AntialiasedLineEnable = false;
	//rasterDesc.CullMode = cull;
	//rasterDesc.DepthBias = 0;
	//rasterDesc.DepthBiasClamp = 0.0f;
	//rasterDesc.DepthClipEnable = true;
	//rasterDesc.FillMode = fill;
	//rasterDesc.FrontCounterClockwise = false;
	//rasterDesc.MultisampleEnable = multisample;
	//rasterDesc.ScissorEnable = false;
	//rasterDesc.SlopeScaledDepthBias = 0.0f;

	std::shared_ptr<Rasterizer> shared(new Rasterizer(this, desc));
	mRasterizers.emplace(key, shared);

	return Rasterizer::Ptr(shared);
}

Renderer::Texture2D::Ptr Renderer::createDepthStencil(int width, int height, DXGI_FORMAT format, bool access )
{
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format =  format;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | (access? D3D11_BIND_SHADER_RESOURCE: 0);
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;


	
	return createTexture(descDepth);
}

Renderer::DepthStencilState::Weak Renderer::createDepthStencilState(const std::string & name, const D3D11_DEPTH_STENCIL_DESC & desc)
{
	auto ret = mDepthStencilStates.find(name);
	if (ret != mDepthStencilStates.end())
		return ret->second;
	ID3D11DepthStencilState* state;
	checkResult(mDevice->CreateDepthStencilState(&desc, &state));
	auto newstate = mDepthStencilStates.emplace(name, new DepthStencilState(state));

	return newstate.first->second;
}

Renderer::Profile::Ptr Renderer::createProfile()
{
	mProfiles.push_back(std::shared_ptr<Profile>(new Profile(this)));
	return mProfiles.back();
}



Renderer::Buffer::Buffer(Renderer* r, const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* data):D3DObject(r)
{
	mDesc = desc;
	checkResult(getDevice()->CreateBuffer(&desc, data, &mBuffer));
}

Renderer::Buffer::Buffer(Renderer * r, int size, int stride, DXGI_FORMAT format, size_t bindflags, D3D11_USAGE usage, size_t cpuaccess) :D3DObject(r)
{
	mDesc = { 0 };
	mDesc.BindFlags = bindflags;
	mDesc.ByteWidth = size;
	mDesc.CPUAccessFlags = cpuaccess;
	mDesc.MiscFlags = (format == DXGI_FORMAT_UNKNOWN) ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0;
	mDesc.StructureByteStride = stride;
	mDesc.Usage = usage;

	checkResult(getDevice()->CreateBuffer(&mDesc, nullptr, &mBuffer));


	bool unorderedAccess = ((bindflags & D3D11_BIND_UNORDERED_ACCESS) != 0);
	bool shaderRecource = ((bindflags & D3D11_BIND_SHADER_RESOURCE) != 0);

	if (shaderRecource)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
		srDesc.Format = format;
		srDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srDesc.Buffer.FirstElement = 0;
		srDesc.Buffer.NumElements = size / stride;
		ID3D11ShaderResourceView* srv;
		checkResult(getDevice()->CreateShaderResourceView(mBuffer, &srDesc, &srv));

		ShaderResource::addView(srv);
	}

	if (unorderedAccess)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
		ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		DescUAV.Format = format;
		DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		DescUAV.Buffer.FirstElement = 0;
		DescUAV.Buffer.NumElements = size / stride;

		ID3D11UnorderedAccessView* uav;
		checkResult(getDevice()->CreateUnorderedAccessView(mBuffer, &DescUAV, &uav));
		UnorderedAccess::addView(uav);
	}

}

Renderer::Buffer::~Buffer()
{
	mBuffer->Release();
}

void Renderer::Buffer::blit(const void * data, size_t size)
{
	if (  (mDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE))
	{
		D3D11_MAP map;
		D3D11_MAPPED_SUBRESOURCE subresource;
		map = (mDesc.Usage & D3D11_USAGE_DYNAMIC) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
		getContext()->Map(mBuffer, 0, map, 0,&subresource);
		memcpy(subresource.pData, data, size);
		getContext()->Unmap(mBuffer, 0);
	}
	else
	{
		getContext()->UpdateSubresource(mBuffer, 0, NULL, data, 0, 0);
	}
}

void Renderer::Buffer::map()
{
	mCache.clear();
}

void Renderer::Buffer::unmap()
{
	blit(mCache.data(), mCache.size());
}

void Renderer::Buffer::write(const void * data, size_t size)
{
	auto f4count = size / 16;
	auto left = size % 16;

	if (f4count != 0 && left != 0)
		error("do not cross the float4(single register) boundary");

	auto curSize = mCache.size();

	auto curf4count = curSize / 16;
	auto curf4left = curSize % 16;

	auto nextf4count = (curSize + size) / 16;
	auto nextf4left = (curSize + size) % 16;

	if (curf4count != nextf4count && nextf4left != 0)
		error("do not cross the float4(single register) boundary");

	const char* beg = (const char*)data;
	const char* end = (const char*)data + size;
	for (; beg != end; ++beg)
		mCache.emplace_back(*beg);

}

void Renderer::Buffer::skip(size_t size)
{
	auto skipleft = (mCache.size() + size) % 16;
	if (skipleft != 0)
		error("next data should be at the beginning of a single register");

	mCache.resize(mCache.size() + size);
}



Renderer::Effect::Effect(Renderer* renderer,const void* compiledshader, size_t size) :D3DObject(renderer)
{
	mCompiledShader.resize(size);
	memcpy(mCompiledShader.data(), compiledshader, size);
	checkResult(D3DX11CreateEffectFromMemory(compiledshader, size, 0, getDevice(), &mEffect));

	D3DX11_EFFECT_DESC desc;
	checkResult(mEffect->GetDesc(&desc));
	for (int i = 0; i < desc.Techniques; ++i)
	{
		auto tech = mEffect->GetTechniqueByIndex(i);
		D3DX11_TECHNIQUE_DESC desc;
		tech->GetDesc(&desc);
		std::string name;
		if (desc.Name != NULL)
			name = desc.Name;
		mTechs.emplace(name, tech);
	}
}

Renderer::Effect::~Effect()
{
	mEffect->Release();
}

void Renderer::Effect::render(Renderer::Ptr renderer, std::function<void(ID3DX11EffectPass*)> draw)
{
	if (!draw)
	{
		return abort();
	}
	auto endi = mTechs.end();
	auto best = mTechs.find(mSelectedTech);
	if (best == endi)
		best = mTechs.begin();

	if (best == endi)
		return;

	auto tech = best->second;
	D3DX11_TECHNIQUE_DESC desc;
	best->second->GetDesc(&desc);

	auto context = renderer->getContext();
	for (size_t i = 0; i < desc.Passes; ++i)
	{
		auto pass = tech->GetPassByIndex(i);
		pass->Apply(0, context);
		draw(pass);
	}
}

void Renderer::Effect::setTech(const std::string & name)
{
	mSelectedTech = name;
}

ID3DX11EffectTechnique * Renderer::Effect::getTech(const std::string & name)
{
	auto ret = mTechs.find(name);
	if (ret == mTechs.end())
		return nullptr;
	return ret->second;
}

ID3DX11EffectVariable * Renderer::Effect::getVariable(const std::string & name)
{
	return mEffect->GetVariableByName(name.c_str());
}

ID3DX11EffectConstantBuffer * Renderer::Effect::getConstantBuffer(const std::string & name)
{
	return mEffect->GetConstantBufferByName(name.c_str());
}



Renderer::Texture::~Texture()
{
}


//Renderer::Texture::Texture(Renderer* renderer, const D3D11_TEXTURE2D_DESC& desc, const void* data, size_t size) :D3DObject(renderer)
//{
//	mDesc = desc;
//	D3D11_SUBRESOURCE_DATA initdata;
//	initdata.pSysMem = data;
//	initdata.SysMemPitch = size;
//	initdata.SysMemSlicePitch = 0;
//	checkResult(getDevice()->CreateTexture2D(&desc,  nullptr, &mTexture));
//
//
//	initTexture();
//
//}
//Renderer::Texture::Texture(Renderer* r, const std::string& filename):D3DObject(r)
//{
//	checkResult(D3DX11CreateTextureFromFileA(getDevice(), filename.c_str(), NULL, NULL, (ID3D11Resource**)&mTexture, NULL));
//	initTexture();
//}
//
//Renderer::Texture::Texture(Renderer * r, ID3D11Texture2D * t) :D3DObject(r)
//{
//	mTexture = t;
//	initTexture();
//}


Renderer::Layout::Layout(Renderer* renderer, const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count): D3DObject(renderer)
{
	mElements.resize(count);
	memcpy(mElements.data(), descarray, count * sizeof(D3D11_INPUT_ELEMENT_DESC));

	mSize = 0;
	for (int i = 0; i < count; ++i)
	{
		auto& d = descarray[i];
		mSize += D3D11Helper::sizeof_DXGI_FORMAT(d.Format);
	}
}





ID3D11InputLayout * Renderer::Layout::bind(ID3DX11EffectPass* pass)
{
	auto ret = mBindedLayouts.find(pass);
	if (ret != mBindedLayouts.end())
		return *ret->second;

	D3DX11_PASS_DESC desc;
	pass->GetDesc(&desc);

	ID3D11InputLayout* layout;
	checkResult(getDevice()->CreateInputLayout(mElements.data(), mElements.size(), desc.pIAInputSignature, desc.IAInputSignatureSize, &layout));

	auto insert = mBindedLayouts.emplace(pass, new Interface<ID3D11InputLayout>(layout));

	return *insert.first->second;
}

ID3D11InputLayout * Renderer::Layout::bind(VertexShader::Weak vs)
{
	auto ptr = vs.lock();
	if (ptr == nullptr)
		return nullptr;
	
	const auto& shader = ptr->getCompiledShader();
	auto ret = mBindedLayouts.find(shader.data());
	if (ret != mBindedLayouts.end())
		return *ret->second;

	ID3D11InputLayout* layout;
	checkResult(getDevice()->CreateInputLayout(mElements.data(), mElements.size(), shader.data(), shader.size(), &layout));

	auto insert = mBindedLayouts.emplace(shader.data(), new Interface<ID3D11InputLayout>(layout));

	return *insert.first->second;
}

Renderer::Font::Font(Renderer* renderer, const std::wstring & font):D3DObject(renderer)
{
	try {
		mFont = std::make_unique<DirectX::SpriteFont>(getDevice(), font.c_str());
		mBatch = std::make_unique<DirectX::SpriteBatch>(getContext());
	}
	catch (std::exception& e)
	{
		error(e.what());
	}
}

Renderer::Font::~Font()
{
}

void Renderer::Font::drawText(const std::string & text, const DirectX::SimpleMath::Vector2 & pos, const DirectX::SimpleMath::Color & color, float rotation, const DirectX::SimpleMath::Vector2 & origin, float scale, DirectX::SpriteEffects effect, float layerdepth)
{
	try {
		getRenderer()->setRenderTarget(mTarget);
		getRenderer()->setViewport({ 0,0, (float)mTarget->getDesc().Width, (float)mTarget->getDesc().Height,0,1.0f });

		mBatch->Begin();
		mFont->DrawString(mBatch.get(), text.c_str(), pos, color, rotation, origin, scale, effect, layerdepth);
		mBatch->End();

		getRenderer()->removeRenderTargets();
	}
	catch (std::exception& e)
	{
		MessageBoxA(NULL, e.what(), NULL, NULL);
		abort();
	}
}

Renderer::Rasterizer::Rasterizer(Renderer* renderer, const D3D11_RASTERIZER_DESC & desc):D3DObject(renderer)
{
	checkResult(getDevice()->CreateRasterizerState(&desc, &mRasterizer));
}

Renderer::Rasterizer::~Rasterizer()
{
	mRasterizer->Release();
}


Renderer::Profile::Profile(Renderer * r):D3DObject(r)
{
	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;
	desc.Query = D3D11_QUERY_TIMESTAMP;

	checkResult(getDevice()->CreateQuery(&desc, &mBegin));
	checkResult(getDevice()->CreateQuery(&desc, &mEnd));

}

Renderer::Profile::~Profile()
{
	mBegin->Release();
	mEnd->Release();
}

Renderer::Profile::Timer Renderer::Profile::count()
{
	return Timer(this);
}

void Renderer::Profile::getData(UINT64 frq)
{
	UINT64 beg, end;
	getContext()->GetData(mBegin, &beg, sizeof(UINT64), 0);
	getContext()->GetData(mEnd, &end, sizeof(UINT64), 0);

	float delta = float(end - beg) / float(frq) * 1000.0f;

	mGPUElapsedTime = delta;
	//mCachedTime += delta;
	//mNumCached++;

	//if (mNumCached < 10)
	//	return;

	//mElapsedTime = mCachedTime / (float)mNumCached;
	//mCachedTime = 0;
	//mNumCached = 0;
}

void Renderer::Profile::begin()
{
	mCPUTimer = GetTickCount64();
	getContext()->End(mBegin);
}

void Renderer::Profile::end()
{
	mCPUElapsedTime = (GetTickCount64() - mCPUTimer) ;
	getContext()->End(mEnd);
}

std::string Renderer::Profile::toString()const
{
	std::stringstream ss;
	ss.precision(4);
	ss << getCPUElapsedTime() << "(" << getGPUElapsedTime() << ")";
	return ss.str();
}


Renderer::Profile::Timer::Timer(Profile * p) :profile(p)
{
#ifdef USE_PROFILE
	p->begin();
#endif
}

Renderer::Profile::Timer::~Timer()
{
#ifdef USE_PROFILE
	profile->end();
#endif
}


void Renderer::Texture2D::initTexture()
{
	bool unorderedAccess = ((mDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0);
	bool shaderRecource = ((mDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0);
	bool rendertarget = ((mDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0);
	bool depthstencil = ((mDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0);

	if (rendertarget && depthstencil)
		error("cannot support a texture with rtview and dsview");


	if (rendertarget)
	{
		if (mDesc.ArraySize > 1)
		{
			for (UINT i = 0; i < mDesc.MipLevels; ++i)
			{
				for (UINT j = 0; j < mDesc.ArraySize; ++j)
				{
					D3D11_RENDER_TARGET_VIEW_DESC desc;
					desc.Format = mDesc.Format;
					desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					desc.Texture2DArray.ArraySize = 1;
					desc.Texture2DArray.MipSlice = i;
					desc.Texture2DArray.FirstArraySlice = j;
					ID3D11RenderTargetView* rtv;
					checkResult(getDevice()->CreateRenderTargetView(mTexture, &desc, &rtv));
					RenderTarget::addView(rtv);
				}
			}
		}
		else
		{
			ID3D11RenderTargetView* rtv;
			checkResult(getDevice()->CreateRenderTargetView(mTexture, NULL, &rtv));
			RenderTarget::addView(rtv);
		}
	};

	if (unorderedAccess)
	{
		ID3D11UnorderedAccessView* uav;
		checkResult(getDevice()->CreateUnorderedAccessView(mTexture, nullptr, &uav));
		UnorderedAccess::addView(uav);
	}

	if (depthstencil && shaderRecource)
	{
		DXGI_FORMAT SRVfmt = DXGI_FORMAT_R32_FLOAT;
		DXGI_FORMAT DSVfmt = DXGI_FORMAT_D32_FLOAT;

		switch (mDesc.Format)
		{
		case DXGI_FORMAT_R32_TYPELESS:
			SRVfmt = DXGI_FORMAT_R32_FLOAT;
			DSVfmt = DXGI_FORMAT_D32_FLOAT;
			break;
		case DXGI_FORMAT_R24G8_TYPELESS:
			SRVfmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			DSVfmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		case DXGI_FORMAT_R16_TYPELESS:
			SRVfmt = DXGI_FORMAT_R16_UNORM;
			DSVfmt = DXGI_FORMAT_D16_UNORM;
			break;
		case DXGI_FORMAT_R8_TYPELESS:
			SRVfmt = DXGI_FORMAT_R8_UNORM;
			DSVfmt = DXGI_FORMAT_R8_UNORM;
			break;
		default:
			{
				error("unknown readable depthstencil format , it must be XXXX_TYPELESS");
			}
		}

		ID3D11DepthStencilView* dsv;
		ID3D11ShaderResourceView* srv;
		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = DSVfmt;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;
		checkResult(getDevice()->CreateDepthStencilView(mTexture, &descDSV, &dsv));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		srvd.Format = SRVfmt;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MostDetailedMip = 0;
		srvd.Texture2D.MipLevels = 1;

		checkResult(getDevice()->CreateShaderResourceView(mTexture, &srvd, &srv));

		ShaderResource::addView(srv);
		DepthStencil::addView(dsv);
	}
	else
	{
		if (depthstencil)
		{
			ID3D11DepthStencilView* dsv;
			checkResult(getDevice()->CreateDepthStencilView(mTexture, nullptr, &dsv));
			DepthStencil::addView(dsv);
		}
		if (shaderRecource)
		{
			if (mDesc.MiscFlags == D3D11_RESOURCE_MISC_TEXTURECUBE)
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC desc;
				desc.Format = mDesc.Format;
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				desc.TextureCube.MipLevels = mDesc.MipLevels;
				desc.TextureCube.MostDetailedMip = 0;
				ID3D11ShaderResourceView* srv;
				checkResult(getDevice()->CreateShaderResourceView(mTexture, &desc, &srv));
				ShaderResource::addView(srv);

			}
			else
			{
				ID3D11ShaderResourceView* srv;
				checkResult(getDevice()->CreateShaderResourceView(mTexture, nullptr, &srv));
				ShaderResource::addView(srv);

			}
		}
	}

}

void Renderer::Texture2D::blit(const void * data, size_t size, UINT index )
{
	if ((mDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE))
	{
		auto pitch = D3D11Helper::sizeof_DXGI_FORMAT(mDesc.Format)  * mDesc.Width;
		D3D11_MAP map;
		D3D11_MAPPED_SUBRESOURCE subresource;
		map = (mDesc.Usage & D3D11_USAGE_DYNAMIC) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
		getContext()->Map(mTexture, index, map, 0, &subresource);

		//the pitch of mapped memory may be different than the pitch of the texture
		for (int i = 0; i < mDesc.Height; ++i)
		{
			memcpy(( char*)subresource.pData + subresource.RowPitch * i, (const char*)data + pitch * i, pitch);
		}
		
		getContext()->Unmap(mTexture, 0);
	}
	else
	{
		getContext()->UpdateSubresource(mTexture, index, NULL, data, D3D11Helper::sizeof_DXGI_FORMAT(mDesc.Format)  * mDesc.Width, 0);
	}
}


void Renderer::Texture3D::initTexture()
{
	bool unorderedAccess = ((mDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0);
	bool shaderRecource = ((mDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0);
	bool rendertarget = ((mDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0);
	bool depthstencil = ((mDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0);

	if (rendertarget && depthstencil)
		error("cannot support a texture with rtview and dsview");


	if (rendertarget)
	{
		{
			ID3D11RenderTargetView* rtv;
			checkResult(getDevice()->CreateRenderTargetView(mTexture, NULL, &rtv));
			RenderTarget::addView(rtv);
		}
	};

	if (unorderedAccess)
	{
		//ID3D11UnorderedAccessView* uav;
		//checkResult(getDevice()->CreateUnorderedAccessView(mTexture, nullptr, &uav));
		//UnorderedAccess::addView(uav);


		for (UINT i = 0; i < mDesc.MipLevels; ++i)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
			desc.Format = mDesc.Format;
			desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = i;
			desc.Texture3D.FirstWSlice = 0;
			desc.Texture3D.WSize = -1;
			ID3D11UnorderedAccessView* uav;
			checkResult(getDevice()->CreateUnorderedAccessView(mTexture, &desc, &uav));
			UnorderedAccess::addView(uav);
		}
	}

	if (shaderRecource)
	{
		ID3D11ShaderResourceView* srv;
		checkResult(getDevice()->CreateShaderResourceView(mTexture, nullptr, &srv));
		ShaderResource::addView(srv);
	}
}
