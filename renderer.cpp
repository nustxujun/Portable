#include "renderer.h"
#include <array>
#include <D3Dcompiler.h>
#include <fstream>
#include <vector>
#include <algorithm>

#define ERROR(x) {MessageBox(NULL, TEXT(x), NULL, MB_ICONERROR); abort();}

void Renderer::checkResult(HRESULT hr)
{
	if (hr == S_OK) return;
	TCHAR msg[MAX_PATH] = {0};
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, hr, 0, msg, sizeof(msg), 0);
	MessageBox(NULL, msg, NULL, MB_ICONERROR);
	abort();
}


void Renderer::init(HWND win, int width, int height)
{
	mWidth = width;
	mHeight = height;
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
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
#ifdef _DEBUG
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
	ID3D11RenderTargetView* bbv;
	checkResult( mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer) );

	checkResult(mDevice->CreateRenderTargetView(backbuffer, NULL, &bbv));

	auto shared = std::shared_ptr<RenderTarget>(new RenderTarget(backbuffer, bbv, nullptr));
	mRenderTargets.insert(shared);
	mBackbuffer = shared;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	checkResult(mDevice->CreateTexture2D(&descDepth, NULL, &mDepthStencil));
	

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	checkResult(mDevice->CreateDepthStencilView(mDepthStencil, &descDSV, &mDepthStencilView));


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

	checkResult(mDevice->CreateRasterizerState(&rasterDesc, &mRasterizer));
	mContext->RSSetState(mRasterizer);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	mContext->RSSetViewports(1, &vp);

	initRenderTargets();
}

void Renderer::initRenderTargets()
{

}

void Renderer::present()
{
	mSwapChain->Present(0,0);
}

void Renderer::clearDepth(float depth)
{
	mContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, depth, 0);
}

void Renderer::clearStencil(int stencil)
{
	mContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_STENCIL, 1.0f, stencil);
}

void Renderer::clearRenderTarget(RenderTarget::Ptr rt, const float color[4])
{
	auto ptr = rt.lock();
	if (ptr == nullptr) return;
	mContext->ClearRenderTargetView(*ptr, color);
}

void Renderer::setTexture(const Texture::Ptr tex)
{
	auto ptr= tex.lock();
	if (ptr == nullptr)
	{
		ID3D11ShaderResourceView* srv[1] = { nullptr };
		mContext->PSSetShaderResources(0, 1, srv);
	}
	else
	{
		ID3D11ShaderResourceView* srv = *ptr;
		mContext->PSSetShaderResources(0, 1, &srv);
	}
}

void Renderer::setTextures(const std::vector<Texture::Ptr>& texs)
{
	std::vector<ID3D11ShaderResourceView*> list;
	for (auto i : texs)
	{
		if (i.lock() != NULL)
			list.push_back( *i.lock());
	}

	if (list.size() == 0)
	{
		ID3D11ShaderResourceView* srv[1] = { nullptr };
		mContext->PSSetShaderResources(0, 1, srv);
	}
	else
		mContext->PSSetShaderResources(0, list.size(), list.data());
}

void Renderer::setRenderTarget(RenderTarget::Ptr rt)
{
	auto ptr = rt.lock();
	if (ptr == nullptr)
	{
		ID3D11RenderTargetView* nil[1] = { NULL };
		mContext->OMSetRenderTargets( 1, nil, mDepthStencilView);
	}
	else
	{
		ID3D11RenderTargetView* rtv = *ptr;
		mContext->OMSetRenderTargets(1, &rtv, mDepthStencilView);
	}
}

void Renderer::setRenderTargets(std::vector<RenderTarget::Ptr>& rts)
{
	std::vector<ID3D11RenderTargetView*> list;
	for (auto i : rts)
		list.push_back(*i.lock());

	if (list.size() == 0)
	{
		ID3D11RenderTargetView* nil[1] = { NULL };
		mContext->OMSetRenderTargets(1, nil, mDepthStencilView);
	}
	else
		mContext->OMSetRenderTargets(list.size(), list.data(), mDepthStencilView);
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

void Renderer::setVertexBuffer(Buffer::Ptr b, size_t stride, size_t offset)
{
	auto ptr = b.lock();
	if (ptr == nullptr) return;

	ID3D11Buffer* vb = *ptr;
	mContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
}

void Renderer::setVertexBuffers( std::vector<Buffer::Ptr>& b, const size_t * stride, const size_t * offset)
{
	std::vector<ID3D11Buffer*> list;
	for (auto i : b)
		list.push_back(*i.lock());

	mContext->IASetVertexBuffers(0, b.size(), list.data(), stride, offset);
}


void Renderer::uninit()
{
	mSamplers.clear();
	mTextures.clear();
	mEffects.clear();
	mLayouts.clear();
	mRenderTargets.clear();
	mBuffers.clear();
	mRasterizer->Release();
	mDepthStencilView->Release();
	mDepthStencil->Release();
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
		d3dDebug->Release();

		MessageBox(NULL, TEXT("some objects were not released."), NULL, NULL);
	}

}

Renderer::Sampler::Ptr Renderer::createSampler(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV, D3D11_TEXTURE_ADDRESS_MODE addrW, D3D11_COMPARISON_FUNC cmpfunc, float minlod, float maxlod)
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
	auto ret = mSamplers.emplace(new Sampler(sampler));
	return Sampler::Ptr(*ret.first);
}

Renderer::Texture::Ptr Renderer::createTexture(const std::string & filename)
{
	auto exist = mTextures.find(filename);
	if (exist != mTextures.end())
		return exist->second;

	ID3D11ShaderResourceView* srv;
	checkResult( D3DX11CreateShaderResourceViewFromFileA(mDevice, filename.c_str(), NULL, NULL, &srv, NULL));
	auto ret = mTextures.emplace(filename, new Texture(srv));
	return Texture::Ptr(ret.first->second);
}

Renderer::RenderTarget::Ptr Renderer::createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage)
{
	ID3D11Texture2D* texture;
	ID3D11RenderTargetView* rtv;
	ID3D11ShaderResourceView* srv;


	// Create the final texture.
	D3D11_TEXTURE2D_DESC descFinalTexture;
	ZeroMemory(&descFinalTexture, sizeof(descFinalTexture));
	descFinalTexture.Width = width;
	descFinalTexture.Height = height;
	descFinalTexture.MipLevels = 1;
	descFinalTexture.ArraySize = 1;
	descFinalTexture.Format = format;
	descFinalTexture.SampleDesc.Count = 1;
	descFinalTexture.SampleDesc.Quality = 0;
	descFinalTexture.Usage = usage;
	descFinalTexture.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	descFinalTexture.CPUAccessFlags = 0;
	descFinalTexture.MiscFlags = 0;

	checkResult( mDevice->CreateTexture2D(&descFinalTexture, NULL, &texture));

	// Create the final render target.
	D3D11_RENDER_TARGET_VIEW_DESC finalRTVDesc;
	ZeroMemory(&finalRTVDesc, sizeof(finalRTVDesc));
	finalRTVDesc.Format = descFinalTexture.Format;
	finalRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	finalRTVDesc.Texture2D.MipSlice = 0;

	checkResult( mDevice->CreateRenderTargetView(texture, &finalRTVDesc, &rtv));


	// Create the final shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC finalSRVDesc;
	finalSRVDesc.Format = descFinalTexture.Format;
	finalSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	finalSRVDesc.Texture2D.MostDetailedMip = 0;
	finalSRVDesc.Texture2D.MipLevels = 1;

	checkResult( mDevice->CreateShaderResourceView(texture, &finalSRVDesc, &srv) );

	auto ret = mRenderTargets.emplace(new RenderTarget(texture, rtv, srv));

	return RenderTarget::Ptr(*ret.first);
}

Renderer::Buffer::Ptr Renderer::createBuffer(int size, D3D11_BIND_FLAG flag, const D3D11_SUBRESOURCE_DATA* initialdata, D3D11_USAGE usage, bool CPUaccess)
{
	ID3D11Buffer* buffer;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = usage;
	bd.ByteWidth = size;
	bd.BindFlags = flag;
	bd.CPUAccessFlags = CPUaccess;
	checkResult(mDevice->CreateBuffer(&bd, initialdata, &buffer));
	auto ret = mBuffers.emplace(new Buffer(buffer));
	return Buffer::Ptr(*ret.first);
}

Renderer::SharedCompiledData Renderer::compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro)
{
	int flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	ID3D10Blob* blob;
	ID3D10Blob* err;
	HRESULT hr = D3DX11CompileFromFileA(filename.c_str(), macro, NULL, entryPoint.c_str(), shaderModel.c_str(), flags, 0, NULL, &blob, &err, NULL);
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
	return SharedCompiledData(new CompiledData(blob));
}

Renderer::Effect::Ptr Renderer::createEffect(void* data, size_t size)
{
	auto ret = mEffects.emplace( new Effect(mDevice,data, size));
	return Effect::Ptr(*ret.first);
}

Renderer::Layout::Ptr Renderer::createLayout(const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count)
{
	auto ret = mLayouts.emplace(new Layout(mDevice, descarray, count));
	return Layout::Ptr(*ret.first);
}

Renderer::RenderTarget::RenderTarget(ID3D11Texture2D * t, ID3D11RenderTargetView * rtv, ID3D11ShaderResourceView * srv):
	mTexture(t), mRTView(rtv), mSRView(srv)
{

}

Renderer::RenderTarget::~RenderTarget()
{

	mTexture->Release();
	mRTView->Release();
	if (mSRView )
		mSRView->Release();
}

Renderer::Buffer::Buffer(ID3D11Buffer * b):mBuffer(b)
{
}

Renderer::Buffer::~Buffer()
{
	mBuffer->Release();
}



Renderer::Effect::Effect(ID3D11Device * d,const void* compiledshader, size_t size) :D3DObject(d)
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
		mTechs.emplace(desc.Name, tech);
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

Renderer::Layout::Layout(ID3D11Device * device, const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count): D3DObject(device)
{
	mElements.resize(count);
	memcpy(mElements.data(), descarray, count * sizeof(D3D11_INPUT_ELEMENT_DESC));
}

ID3D11InputLayout * Renderer::Layout::bind(const char* data, size_t size)
{
	std::hash<std::string> hash;
	auto key = hash(std::string(data, size));

	auto ret = mBindedLayouts.find(key);
	if (ret != mBindedLayouts.end())
		return *ret->second;

	ID3D11InputLayout* layout;
	checkResult(getDevice()->CreateInputLayout(mElements.data(), mElements.size(), data, size, &layout));

	auto insert = mBindedLayouts.emplace(layout, new Interface<ID3D11InputLayout>(layout));

	return *insert.first->second;
}
