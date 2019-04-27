#include "renderer.h"
#include <array>
#include <D3Dcompiler.h>
#include <fstream>
#include <vector>

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
	checkResult( mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer) );

	checkResult(mDevice->CreateRenderTargetView(backbuffer, NULL, &mBackbufferView));
	backbuffer->Release();

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

	initRenderTargets();
}

void Renderer::initRenderTargets()
{

}

void Renderer::present()
{
	mSwapChain->Present(0,0);
}

void Renderer::uninit()
{
	mEffects.clear();
	mLayouts.clear();
	mRenderTargets.clear();
	mBuffers.clear();
	mRasterizer->Release();
	mDepthStencilView->Release();
	mDepthStencil->Release();
	mBackbufferView->Release();
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

Renderer::Buffer::Ptr Renderer::createBuffer(int size, D3D11_BIND_FLAG flag, D3D11_USAGE usage, bool CPUaccess)
{
	ID3D11Buffer* buffer;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = usage;
	bd.ByteWidth = size;
	bd.BindFlags = flag;
	bd.CPUAccessFlags = CPUaccess;
	checkResult(mDevice->CreateBuffer(&bd, NULL, &buffer));
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
		MessageBoxA(NULL, (LPCSTR)err->GetBufferPointer(), NULL, NULL);
		err->Release();
		abort();
	}
	return SharedCompiledData(new CompiledData(blob));
}

Renderer::Effect::Ptr Renderer::createEffect(ID3DBlob* buffer)
{
	ID3DX11Effect* d3deffect;
	checkResult(D3DX11CreateEffectFromMemory(buffer->GetBufferPointer(), buffer->GetBufferSize(), 0, mDevice, &d3deffect));

	auto ret = mEffects.emplace( new Effect(d3deffect));
	return Effect::Ptr(*ret.first);
}

Renderer::Layout::Ptr Renderer::createLayout(const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count, ID3DBlob * buffer)
{
	ID3D11InputLayout* layout;
	checkResult(mDevice->CreateInputLayout(descarray, count, buffer->GetBufferPointer(), buffer->GetBufferSize(), &layout));
	auto ret = mLayouts.emplace(new Layout(layout));
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
	mSRView->Release();
}

Renderer::Buffer::Buffer(ID3D11Buffer * b):mBuffer(b)
{
}

Renderer::Buffer::~Buffer()
{
	mBuffer->Release();
}



Renderer::Effect::Effect(ID3DX11Effect * eff) :mEffect(eff)
{
	int index = 0;
	while (true)
	{
		auto tech = mEffect->GetTechniqueByIndex(index++);
		if (tech == NULL || !tech->IsValid())
			break;
		D3DX11_TECHNIQUE_DESC desc;
		tech->GetDesc(&desc);
		mTechs.emplace(desc.Name, tech);
	}
}

Renderer::Effect::~Effect()
{
	mEffect->Release();
}

void Renderer::Effect::render(ID3D11DeviceContext* context, std::function<void(ID3DX11EffectTechnique*, int)> prepare, std::function<void(int)> draw)
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
	for (size_t i = 0; i < desc.Passes; ++i)
	{
		if (prepare)
			prepare(tech, i);
		tech->GetPassByIndex(i)->Apply(0, context);
		draw(i);
	}
}

void Renderer::Effect::setTech(const std::string & name)
{
	mSelectedTech = name;
}

ID3DX11EffectVariable * Renderer::Effect::getVariable(const std::string & name)
{
	return mEffect->GetVariableByName(name.c_str());
}

ID3DX11EffectConstantBuffer * Renderer::Effect::getConstantBuffer(const std::string & name)
{
	return mEffect->GetConstantBufferByName(name.c_str());
}

