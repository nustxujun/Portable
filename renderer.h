#pragma once

#include <algorithm>
#include "d3dx11effect.h"
#include <D3D11.h>
#include <D3DX11.h>
#include "Common.h"
#include "SpriteFont.h"
#include "SimpleMath.h"
#undef min
#undef max

class Renderer
{
public: 
	using Ptr = std::shared_ptr<Renderer>;
private:

	class NODefault
	{
	public:
		NODefault() = default;
		NODefault(NODefault&) = delete;
		NODefault(NODefault&&) = delete;
		void operator=(NODefault&) = delete;

		bool operator<(const NODefault& a)const
		{
			return this < &a;
		}
	};

	class D3DObject
	{
	public:
		D3DObject(ID3D11Device* device): mDevice(device){}
	protected:
		ID3D11Device* getDevice()const { return mDevice; }
		ID3D11DeviceContext* getContext()const {
			ID3D11DeviceContext* c;
			mDevice->GetImmediateContext(&c);
			c->Release();
			return c;
		}
	private:
		ID3D11Device* mDevice;
	};
public:
	template<class T>
	class Interface final : public NODefault
	{
	public:
		using Shared = std::shared_ptr<Interface<T>>;
		using Weak = std::weak_ptr<Interface<T>>;

		Interface(T* t) : mInterface(t) {}
		~Interface() { if (mInterface) mInterface->Release(); }
		T* operator->() { return mInterface; }
		operator T*() { return mInterface; }
	private:
		T* mInterface;
	};

	class RenderTarget final : public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<RenderTarget>;
	public:
		RenderTarget(ID3D11Device* d, int width, int height, DXGI_FORMAT format, D3D11_USAGE usage);
		RenderTarget(ID3D11Device* d, ID3D11Texture2D* t, ID3D11RenderTargetView* rt, ID3D11ShaderResourceView* srv);
		~RenderTarget();
		operator ID3D11RenderTargetView*() { return mRTView; }
		
		ID3D11Texture2D* getTexture() const{ return mTexture; }
		ID3D11ShaderResourceView* getShaderResourceView()const { return mSRView; }
		ID3D11RenderTargetView* getRenderTargetView() const{ return mRTView; }

		void clear(const std::array<float,4> c);
	private:
		ID3D11Texture2D* mTexture;
		ID3D11RenderTargetView* mRTView;
		ID3D11ShaderResourceView* mSRView;
	};

	class Buffer final : public NODefault, public D3DObject
	{
	public: 
		using Ptr = std::weak_ptr<Buffer>;
	public:
		Buffer(ID3D11Device * d, const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* data);
		~Buffer();

		void blit(const void* data, size_t size);


		operator ID3D11Buffer* ()const {return mBuffer;}
	private:
		ID3D11Buffer* mBuffer;
		D3D11_BUFFER_DESC mDesc;
	};

	class Effect final: public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<Effect>;
	public:
		Effect(ID3D11Device* d,const void* compiledshader, size_t size);
		~Effect();
		void render(Renderer::Ptr renderer, std::function<void(ID3DX11EffectPass*)> draw);

		const std::vector<char>& getCompiledShader() { return mCompiledShader; };
		void setTech(const std::string& name);
		ID3DX11EffectTechnique* getTech(const std::string& name);
		operator ID3DX11Effect*() { return mEffect; }

		ID3DX11EffectVariable* getVariable(const std::string& name);
		ID3DX11EffectConstantBuffer * getConstantBuffer(const std::string& name);

	private:
		std::vector<char> mCompiledShader;
		ID3DX11Effect* mEffect;
		std::string mSelectedTech;
		std::unordered_map<std::string, ID3DX11EffectTechnique*> mTechs;
	};

	class Texture final : public NODefault
	{
	public:
		using Ptr = std::weak_ptr<Texture>;
	public:
		Texture(ID3D11ShaderResourceView* srv) :mSRV(srv) {};
		~Texture() { mSRV->Release(); };

		operator ID3D11ShaderResourceView*()const
		{
			return mSRV;
		}
	private:
		ID3D11ShaderResourceView* mSRV;
	};

	class Sampler final : public NODefault
	{
	public :
		using Ptr = std::weak_ptr<Sampler>;
	public:
		Sampler(ID3D11SamplerState* s): mSampler(s){}
		~Sampler() { mSampler->Release(); }

		operator ID3D11SamplerState*()const
		{
			return mSampler;
		}
	private:
		ID3D11SamplerState* mSampler;
	};

	class VertexShader :public NODefault
	{
	public:
		using Shared = std::shared_ptr<VertexShader>;
		using Weak = std::weak_ptr<VertexShader>;

	public:
		VertexShader(ID3D11VertexShader* vs, const void* data, size_t size) :mVS(vs) {
			mCompiledShader.resize(size);
			memcpy(mCompiledShader.data(), data, size);
		}
		~VertexShader() { mVS->Release(); }

		operator ID3D11VertexShader*() const { return mVS; }
		const std::vector<char>& getCompiledShader() { return mCompiledShader; }
	private:
		ID3D11VertexShader* mVS;
		std::vector<char> mCompiledShader;
	};

	class Layout final : public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<Layout>;
	public:
		Layout(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count);
		~Layout(){}

		ID3D11InputLayout* bind(ID3DX11EffectPass* pass);
		ID3D11InputLayout* bind(VertexShader::Weak vs);

		size_t getSize()const { return mSize; };
	private:
		std::unordered_map<const void*, Interface<ID3D11InputLayout>::Shared> mBindedLayouts;
		std::vector<D3D11_INPUT_ELEMENT_DESC> mElements;
		size_t mSize;
	};

	class Font final :public NODefault, public D3DObject
	{
	public :
		using Ptr = std::weak_ptr<Font>;

	public:
		Font(ID3D11Device* d, const std::wstring& font);
		~Font();

		void drawText(
			const std::string& text, 
			const DirectX::SimpleMath::Vector2& pos, 
			const DirectX::SimpleMath::Color& color = (DirectX::FXMVECTOR)DirectX::Colors::White, 
			float rotation = 0, 
			const DirectX::SimpleMath::Vector2& origin = DirectX::SimpleMath::Vector2::Zero,
			float scale = 1,
			DirectX::SpriteEffects effect = DirectX::SpriteEffects_None,
			float layerdepth = 0);
	private:
		std::unique_ptr<DirectX::SpriteFont> mFont;
		std::unique_ptr<DirectX::SpriteBatch> mBatch;
		float x = 0;
		float y = 0;

	};

	class Rasterizer:public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<Rasterizer>;
	public:
		Rasterizer(ID3D11Device* d, const D3D11_RASTERIZER_DESC& desc);
		~Rasterizer();

		operator ID3D11RasterizerState*()const { return mRasterizer; }
	private:
		D3D11_RASTERIZER_DESC mDesc;
		ID3D11RasterizerState* mRasterizer;
	};

	class DepthStencil:public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<DepthStencil>;
	public:
		DepthStencil(ID3D11Device* d, const D3D11_TEXTURE2D_DESC& desc);
		~DepthStencil();

		operator ID3D11DepthStencilView* () const { return mDSView; }
		void clearDepth(float d);
		void clearStencil(int s);
	private:
		ID3D11Texture2D* mTexture;
		ID3D11DepthStencilView* mDSView;
	};


	using CompiledData = Interface<ID3D10Blob>;
	using SharedCompiledData = CompiledData::Shared ;
	using PixelShader = Interface<ID3D11PixelShader>;
	using DepthStencilState = Interface<ID3D11DepthStencilState>;
	using BlendState = Interface<ID3D11BlendState>;

public:
	void init(HWND win, int width, int height);
	void uninit();

	int getWidth()const { return mWidth; }
	int getHeight()const { return mHeight; }

	ID3D11DeviceContext* getContext() { return mContext; };

	void present();
	void clearRenderTarget(RenderTarget::Ptr rt, const float color[4]);
	void clearRenderTargets(std::vector<RenderTarget::Ptr>& rts, const std::array<float,4>& color);
	void clearDefaultStencil(float d, int s = 0);

	void setTexture(const Texture::Ptr tex);
	void setTextures(const std::vector<Texture::Ptr>& texs);
	void setTexture(const RenderTarget::Ptr tex);
	void setTextures(const std::vector<RenderTarget::Ptr>& texs);
	void setShaderResourceView(ID3D11ShaderResourceView* srv);
	void setShaderResourceViews(const std::vector<ID3D11ShaderResourceView*>& srvs);
	void removeShaderResourceViews();

	void setSampler(Sampler::Ptr s);
	void setSamplers(const std::vector<Sampler::Ptr>& ss);
	void removeSamplers();
	Sampler::Ptr getSampler(const std::string&name);
	void setRenderTarget(RenderTarget::Ptr rt, DepthStencil::Ptr ds = DepthStencil::Ptr());
	void setRenderTargets(const std::vector<RenderTarget::Ptr>& rts, DepthStencil::Ptr ds = DepthStencil::Ptr());
	void removeRenderTargets();
	void setConstantBuffer(Buffer::Ptr b);
	void setConstantsBuffer(const std::vector<Buffer::Ptr>& bs);
	void removeConstantBuffers();
	void setRasterizer(Rasterizer::Ptr r);

	RenderTarget::Ptr getBackbuffer() { return mBackbuffer; }
	void setLayout(ID3D11InputLayout* layout);
	void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY pri);
	void setIndexBuffer(Buffer::Ptr b, DXGI_FORMAT format, int offset);
	void setVertexBuffer(Buffer::Ptr b, UINT stride, UINT offset);
	void setVertexBuffers( std::vector<Buffer::Ptr>& b, const UINT* stride, const UINT* offset);
	void setViewport(const D3D11_VIEWPORT& vp);
	void setDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc);
	void setBlendState(const D3D11_BLEND_DESC& desc, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);
	void setDefaultBlendState();
	void setDefaultDepthStencilState();

	Sampler::Ptr createSampler(const std::string& name, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV,
		D3D11_TEXTURE_ADDRESS_MODE addrW = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_FUNC cmpfunc = D3D11_COMPARISON_NEVER, float minlod = 0, float maxlod = D3D11_FLOAT32_MAX);
	Texture::Ptr createTexture(const std::string& filename);
	RenderTarget::Ptr createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	Buffer::Ptr createBuffer(int size, D3D11_BIND_FLAG flag, const D3D11_SUBRESOURCE_DATA* initialdata = NULL,D3D11_USAGE usage = D3D11_USAGE_DEFAULT, bool CPUaccess = false);
	SharedCompiledData compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro = NULL);
	Effect::Ptr createEffect(void* data, size_t size);
	VertexShader::Weak createVertexShader(const void* data, size_t size);
	PixelShader::Weak createPixelShader(const void* data, size_t size);
	Layout::Ptr createLayout(const D3D11_INPUT_ELEMENT_DESC* descarray, size_t count);
	Font::Ptr createOrGetFont(const std::wstring& font);
	Rasterizer::Ptr createOrGetRasterizer(const std::string & name, D3D11_CULL_MODE cull = D3D11_CULL_BACK, D3D11_FILL_MODE fill = D3D11_FILL_SOLID, bool multisample = false);
	DepthStencil::Ptr createDepthStencil(int width, int height, DXGI_FORMAT format);
 private:
	static void checkResult(HRESULT hr);

private:
	int mWidth = 0;
	int mHeight = 0;

	ID3D11Device* mDevice;
	ID3D11DeviceContext* mContext;
	IDXGISwapChain* mSwapChain;
	D3D_FEATURE_LEVEL mFeatureLevel;
	DepthStencil::Ptr mDefaultDepthStencil;
	Rasterizer::Ptr mRasterizer;

	RenderTarget::Ptr mBackbuffer;

	size_t mSamplersState = 0;
	size_t mSRVState = 0;
	size_t mConstantState = 0;

	std::set<std::shared_ptr<RenderTarget>> mRenderTargets;
	std::set<std::shared_ptr<Buffer>> mBuffers;
	std::set<std::shared_ptr<Effect>> mEffects;
	std::set<std::shared_ptr<Layout>> mLayouts;
	std::unordered_map<std::string,std::shared_ptr<Texture>> mTextures;
	std::unordered_map<std::string,std::shared_ptr<Sampler>> mSamplers;
	std::vector<VertexShader::Shared> mVSs;
	std::vector<PixelShader::Shared> mPSs;
	std::unordered_map<std::wstring, std::shared_ptr<Font>> mFonts;
	std::unordered_map<std::string, std::shared_ptr<Rasterizer>> mRasterizers;

	std::unordered_map<size_t, DepthStencilState::Shared> mDepthStencilStates;
	std::unordered_map<size_t, BlendState::Shared> mBlendStates;
	std::unordered_map<size_t, std::shared_ptr<DepthStencil>> mDepthStencils;
};



