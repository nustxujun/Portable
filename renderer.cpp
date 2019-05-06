#include "renderer.h"
#include <array>
#include <D3Dcompiler.h>
#include <fstream>
#include "D3D11Helper.h"


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
	auto shared = std::shared_ptr<RenderTarget>(new RenderTarget(mDevice, backbuffer, bbv, nullptr));
	mRenderTargets.insert(shared);
	mBackbuffer = shared;

	mDefaultDepthStencil = createDepthStencil(width, height, DXGI_FORMAT_D24_UNORM_S8_UINT);


	createOrGetRasterizer("default",D3D11_CULL_BACK);

}


void Renderer::present()
{
	mSwapChain->Present(0,0);
}

void Renderer::clearRenderTarget(RenderTarget::Ptr rt, const float color[4])
{
	auto ptr = rt.lock();
	if (ptr == nullptr) return;
	mContext->ClearRenderTargetView(*ptr, color);
}

void Renderer::clearRenderTargets(std::vector<RenderTarget::Ptr>& rts, const std::array<float, 4>& color)
{
	for (auto& i : rts)
	{
		mContext->ClearRenderTargetView(*i.lock(), color.data());
	}
}

void Renderer::clearDefaultStencil(float d, int s)
{
	auto ptr = mDefaultDepthStencil.lock();
	ptr->clearDepth(d);
	ptr->clearStencil(0);
}


void Renderer::setTexture(const Texture::Ptr tex)
{
	auto ptr= tex.lock();
	if (ptr != nullptr)
	{
		setShaderResourceView(*ptr);
	}
}

void Renderer::setTextures(const std::vector<Texture::Ptr>& texs)
{
	std::vector<ID3D11ShaderResourceView*> list;
	list.reserve(texs.size());
	for (auto i : texs)
	{
		if (i.lock() != NULL)
			list.push_back( *i.lock());
	}

	setShaderResourceViews(list);
}

void Renderer::setTexture(const RenderTarget::Ptr tex)
{
	auto ptr = tex.lock();
	if (ptr != nullptr)
	{
		setShaderResourceView((*ptr).getShaderResourceView());
	}
}

void Renderer::setTextures(const std::vector<RenderTarget::Ptr>& texs)
{
	std::vector<ID3D11ShaderResourceView*> list;
	list.reserve(texs.size());

	for (auto i : texs)
	{
		if (i.lock() != NULL)
			list.push_back((*i.lock()).getShaderResourceView());
	}

	setShaderResourceViews(list);
}

void Renderer::setShaderResourceView(ID3D11ShaderResourceView * srv)
{
	removeShaderResourceViews();
	mSRVState = 1;
	mContext->PSSetShaderResources(0, 1, &srv);
}

void Renderer::setShaderResourceViews(const std::vector<ID3D11ShaderResourceView*>& srvs)
{
	removeShaderResourceViews();
	mSRVState = srvs.size();
	mContext->PSSetShaderResources(0, srvs.size(), srvs.data());
}

void Renderer::removeShaderResourceViews()
{
	ID3D11ShaderResourceView* nil = 0;
	for (int i = 0; i < mSRVState; ++i)
		mContext->PSSetShaderResources(i, 1, &nil);
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
	if (ptr == nullptr)
	{
		mContext->OMSetRenderTargets(1, 0, 0);
	}
	else
	{
		auto dsv = ds.lock();
		if (dsv == nullptr)
			dsv = mDefaultDepthStencil.lock();
		ID3D11RenderTargetView* rtv = *ptr;
		mContext->OMSetRenderTargets(1, &rtv, *dsv);
	}
}

void Renderer::setRenderTargets(const std::vector<RenderTarget::Ptr>& rts, DepthStencil::Ptr ds)
{
	auto dsv = ds.lock();
	if (dsv == nullptr)
		dsv = mDefaultDepthStencil.lock();

	std::vector<ID3D11RenderTargetView*> list;
	list.reserve(rts.size());

	for (auto i : rts)
		list.push_back(*i.lock());
	
	mContext->OMSetRenderTargets(list.size(), list.data(), *dsv);
}

void Renderer::removeRenderTargets()
{
	mContext->OMSetRenderTargets(0, 0, 0);
}

void Renderer::setConstantBuffer(Buffer::Ptr b)
{
	removeConstantBuffers();
	auto ptr = b.lock();
	if (ptr != nullptr)
	{
		mConstantState = 1;
		ID3D11Buffer* b = *ptr;
		mContext->PSSetConstantBuffers(0, 1, &b);
	}
}

void Renderer::setConstantsBuffer(const std::vector<Buffer::Ptr>& bs)
{
	std::vector<ID3D11Buffer*> list;
	list.reserve(bs.size());

	for (auto i : bs)
	{
		list.push_back(*(i.lock()));
	}

	mConstantState = list.size();
	mContext->PSSetConstantBuffers(0, list.size(), list.data());
}

void Renderer::removeConstantBuffers()
{
	ID3D11Buffer* nil = 0;
	for (size_t i = 0; i < mConstantState; ++i)
		mContext->PSSetConstantBuffers(i, 1, &nil);
	mConstantState = 0;
}

void Renderer::setRasterizer(Rasterizer::Ptr r)
{
	auto ptr = r.lock();
	if (ptr == nullptr)
		return;

	mContext->RSSetState(*ptr);
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
	list.reserve(b.size());

	for (auto i : b)
		list.push_back(*i.lock());

	mContext->IASetVertexBuffers(0, b.size(), list.data(), stride, offset);
}

void Renderer::setViewport(const D3D11_VIEWPORT & vp)
{
	mContext->RSSetViewports(1, &vp);
}

void Renderer::setDepthStencilState(const D3D11_DEPTH_STENCIL_DESC & desc)
{
	size_t hash = Common::hash(desc);
	auto ret = mDepthStencilStates.find(hash);
	if (ret == mDepthStencilStates.end())
	{
		ID3D11DepthStencilState* state;
		checkResult(mDevice->CreateDepthStencilState(&desc, &state));
		auto newstate = mDepthStencilStates.emplace(hash, new DepthStencilState(state));
		ret = newstate.first;
	}

	mContext->OMSetDepthStencilState(*ret->second, 0);
}

void Renderer::setBlendState(const D3D11_BLEND_DESC& desc, const std::array<float, 4>& factor , size_t mask )
{
	size_t hash = Common::hash(desc);
	auto ret = mBlendStates.find(hash);
	if (ret == mBlendStates.end())
	{
		ID3D11BlendState* state;
		checkResult(mDevice->CreateBlendState(&desc, &state));
		auto newstate = mBlendStates.emplace(hash, new BlendState(state));
		ret = newstate.first;
	}

	mContext->OMSetBlendState(*ret->second, factor.data(), mask);
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

void Renderer::uninit()
{
	mDepthStencils.clear();
	mBlendStates.clear();
	mDepthStencilStates.clear();
	mRasterizers.clear();
	mFonts.clear();
	mVSs.clear();
	mPSs.clear();
	mSamplers.clear();
	mTextures.clear();
	mEffects.clear();
	mLayouts.clear();
	mRenderTargets.clear();
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

	auto ret = mRenderTargets.emplace(new RenderTarget(mDevice,  width,  height,  format,  usage));
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
	auto ret = mBuffers.emplace(new Buffer(mDevice, bd, initialdata));
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

Renderer::VertexShader::Weak Renderer::createVertexShader(const void * data, size_t size)
{
	ID3D11VertexShader* vs;
	checkResult(mDevice->CreateVertexShader(data, size, NULL, &vs));
	mVSs.emplace_back(new VertexShader(vs, data, size));
	return mVSs.back();
}

Renderer::PixelShader::Weak Renderer::createPixelShader(const void * data, size_t size)
{
	ID3D11PixelShader* ps;
	checkResult(mDevice->CreatePixelShader(data, size, NULL, &ps));
	mPSs.emplace_back(new PixelShader(ps));
	return mPSs.back();
}

Renderer::Layout::Ptr Renderer::createLayout(const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count)
{
	auto ret = mLayouts.emplace(new Layout(mDevice, descarray, count));
	return Layout::Ptr(*ret.first);
}

Renderer::Font::Ptr Renderer::createOrGetFont(const std::wstring & font)
{
	auto ret = mFonts.find(font);
	if (ret != mFonts.end())
		return ret->second;

	std::shared_ptr<Font> shared (new Font(mDevice,font));
	mFonts.emplace(font, shared);
	return Font::Ptr(shared);
}

Renderer::Rasterizer::Ptr Renderer::createOrGetRasterizer(const std::string & name, D3D11_CULL_MODE cull, D3D11_FILL_MODE fill, bool multisample)
{
	auto ret = mRasterizers.find(name);
	if (ret != mRasterizers.end())
		return ret->second;

	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = cull;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = fill;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = multisample;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	std::shared_ptr<Rasterizer> shared(new Rasterizer(mDevice, rasterDesc));
	mRasterizers.emplace(name, shared);

	return Rasterizer::Ptr(shared);
}

Renderer::DepthStencil::Ptr Renderer::createDepthStencil(int width, int height, DXGI_FORMAT format)
{
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = format;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	size_t key = Common::hash(descDepth);
	auto ret = mDepthStencils.find(key);
	if (ret != mDepthStencils.end())
		return ret->second;

	auto ins = mDepthStencils.emplace(key, std::shared_ptr<DepthStencil>(new DepthStencil(mDevice, descDepth)));
	
	return ins.first->second;
}

Renderer::RenderTarget::RenderTarget(ID3D11Device* d, int width, int height, DXGI_FORMAT format, D3D11_USAGE usage):D3DObject(d)
{
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

	checkResult(d->CreateTexture2D(&descFinalTexture, NULL, &mTexture));

	// Create the final render target.
	D3D11_RENDER_TARGET_VIEW_DESC finalRTVDesc;
	ZeroMemory(&finalRTVDesc, sizeof(finalRTVDesc));
	finalRTVDesc.Format = descFinalTexture.Format;
	finalRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	finalRTVDesc.Texture2D.MipSlice = 0;

	checkResult(d->CreateRenderTargetView(mTexture, &finalRTVDesc, &mRTView));


	// Create the final shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC finalSRVDesc;
	finalSRVDesc.Format = descFinalTexture.Format;
	finalSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	finalSRVDesc.Texture2D.MostDetailedMip = 0;
	finalSRVDesc.Texture2D.MipLevels = 1;

	checkResult(d->CreateShaderResourceView(mTexture, &finalSRVDesc, &mSRView));

}

Renderer::RenderTarget::RenderTarget(ID3D11Device* d, ID3D11Texture2D* t, ID3D11RenderTargetView* rt, ID3D11ShaderResourceView* srv):D3DObject(d)
{
	mTexture = t;
	mRTView = rt;
	mSRView = nullptr;
}


Renderer::RenderTarget::~RenderTarget()
{
	mTexture->Release();
	mRTView->Release();
	if (mSRView)
		mSRView->Release();
}

void Renderer::RenderTarget::clear(const std::array<float, 4> c)
{
	getContext()->ClearRenderTargetView(mRTView, c.data());
}


Renderer::Buffer::Buffer(ID3D11Device * d, const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* data):D3DObject(d)
{
	mDesc = desc;
	checkResult(d->CreateBuffer(&desc, data, &mBuffer));
}

Renderer::Buffer::~Buffer()
{
	mBuffer->Release();
}

void Renderer::Buffer::blit(const void * data, size_t size)
{
	if (mDesc.Usage & D3D11_USAGE_DYNAMIC)
	{
		abort();
	}
	else
	{
		getContext()->UpdateSubresource(mBuffer, 0, NULL, data, 0, 0);
	}
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

Renderer::Layout::Layout(ID3D11Device * device, const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count): D3DObject(device)
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

Renderer::Font::Font(ID3D11Device * d, const std::wstring & font):D3DObject(d)
{
	try {
		mFont = std::make_unique<DirectX::SpriteFont>(d, font.c_str());
		mBatch = std::make_unique<DirectX::SpriteBatch>(getContext());
	}
	catch (std::exception& e)
	{
		MessageBoxA(NULL, e.what(), NULL, NULL);
		abort();
	}
}

Renderer::Font::~Font()
{
}

void Renderer::Font::drawText(const std::string & text, const DirectX::SimpleMath::Vector2 & pos, const DirectX::SimpleMath::Color & color, float rotation, const DirectX::SimpleMath::Vector2 & origin, float scale, DirectX::SpriteEffects effect, float layerdepth)
{
	try {

		ID3D11DepthStencilState* dss;
		UINT ref = 0;
		getContext()->OMGetDepthStencilState(&dss,&ref);

		mBatch->Begin();
		mFont->DrawString(mBatch.get(), text.c_str(), pos, color, rotation, origin, scale, effect, layerdepth);
		mBatch->End();

		getContext()->OMSetDepthStencilState(dss, ref);
	}
	catch (std::exception& e)
	{
		MessageBoxA(NULL, e.what(), NULL, NULL);
		abort();
	}
}

Renderer::Rasterizer::Rasterizer(ID3D11Device * d, const D3D11_RASTERIZER_DESC & desc):D3DObject(d)
{
	checkResult(d->CreateRasterizerState(&desc, &mRasterizer));
}

Renderer::Rasterizer::~Rasterizer()
{
	mRasterizer->Release();
}

Renderer::DepthStencil::DepthStencil(ID3D11Device * d, const D3D11_TEXTURE2D_DESC & desc):D3DObject(d)
{
	checkResult(d->CreateTexture2D(&desc, NULL, &mTexture));
	
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = desc.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	checkResult(d->CreateDepthStencilView(mTexture, &descDSV, &mDSView));
}

Renderer::DepthStencil::~DepthStencil()
{
	mTexture->Release();
	mDSView->Release();
}

void Renderer::DepthStencil::clearDepth(float d)
{
	getContext()->ClearDepthStencilView(mDSView, D3D11_CLEAR_DEPTH, d, 0);
}

void Renderer::DepthStencil::clearStencil(int s)
{
	getContext()->ClearDepthStencilView(mDSView, D3D11_CLEAR_STENCIL, 1.0f, s);
}
