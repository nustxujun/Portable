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
		D3DObject(Renderer* renderer): mRenderer(renderer){}
		D3DObject() { abort(); }
	protected:
		ID3D11Device* getDevice()const { return mRenderer->mDevice; }
		ID3D11DeviceContext* getContext()const { return mRenderer->mContext; }
		Renderer* getRenderer()const { return mRenderer; }
	private:
		Renderer* mRenderer = nullptr;
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

	class ShaderResource : public NODefault, virtual public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<ShaderResource>;
	public:
		ShaderResource() {}
		~ShaderResource();

		ID3D11ShaderResourceView* getShaderResourceView()const { return mSRView; }
		operator ID3D11ShaderResourceView* ()const { return mSRView; }

	protected:
		ID3D11ShaderResourceView* mSRView = NULL;
	};

	class UnorderedAccess : public NODefault, virtual public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<UnorderedAccess>;

	public:
		UnorderedAccess()  {}

		UnorderedAccess(Renderer* r, ID3D11UnorderedAccessView* uav);

		~UnorderedAccess();

		operator ID3D11UnorderedAccessView* ()const { return mUAV; }
		ID3D11UnorderedAccessView* getUnorderedAccess()const { return mUAV; }

		void clear(const std::array<UINT, 4>& value);
		void clear( const std::array<float, 4>& value);

	protected:
		ID3D11UnorderedAccessView* mUAV = NULL;
	};

	class DepthStencil : public NODefault, virtual public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<DepthStencil>;
	public:
		DepthStencil() {};
		~DepthStencil();

		operator ID3D11DepthStencilView* () const { return mDSView; }
		void clearDepth(float d);
		void clearStencil(int s);
		void clearDepthAndStencil(float d, int s);
	protected:
		ID3D11DepthStencilView* mDSView = nullptr;
	};




	class RenderTarget : public NODefault, virtual public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<RenderTarget>;
	public:
		RenderTarget()  {}

		RenderTarget(Renderer* r,ID3D11RenderTargetView* rt);

		~RenderTarget();
		operator ID3D11RenderTargetView*() { return mRTView; }
		ID3D11RenderTargetView* getRenderTargetView() const{ return mRTView; }

		void clear(const std::array<float, 4> c);

	protected:
		ID3D11RenderTargetView* mRTView = nullptr;
	};

	class Buffer : public UnorderedAccess, public ShaderResource
	{
	public: 
		using Ptr = std::weak_ptr<Buffer>;
	public:
		Buffer(Renderer* renderer, int size, int stride, DXGI_FORMAT format,  size_t bindflags, D3D11_USAGE usage, size_t cpuaccess);
		Buffer(Renderer* renderer, const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* data);
		~Buffer();

		template<class T>
		void blit(const T& value)
		{
			blit(&value, sizeof(value));
		}
		void blit(const void* data, size_t size);


		operator ID3D11Buffer* ()const {return mBuffer;}
		ID3D11Buffer* getBuffer()const { return mBuffer; }
	private:
		ID3D11Buffer* mBuffer;
		D3D11_BUFFER_DESC mDesc;
	};

	class Effect final: public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<Effect>;
	public:
		Effect(Renderer* renderer, const void* compiledshader, size_t size);
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

	class Texture : public RenderTarget,public ShaderResource, public UnorderedAccess, public DepthStencil
	{
	public:
		using Ptr = std::weak_ptr<Texture>;
	public:
		virtual ~Texture();
	private:

	};

	template<class T, class D>
	class TextureUnknown : public Texture
	{
	public:
		using Ptr = std::weak_ptr<TextureUnknown>;
	public:
		TextureUnknown(T* tex )
		{
			mTexture = tex;
			tex->GetDesc(&mDesc);
		}

		virtual ~TextureUnknown()
		{
			mTexture->Release();
		}

		const D& getDesc() { return mDesc; }
		operator T*()const { return mTexture; }
		T* getTexture()const { return mTexture; }

		void swap(Ptr t, bool force = false)
		{
			auto ptr = t.lock();
			if (ptr == nullptr) return;
			if (memcmp(&mDesc, &ptr->mDesc, sizeof(mDesc)) != 0 && !force)
			{
				error("only can swap the same textures.");
				return;
			}

			std::swap(mTexture, ptr->mTexture);
			std::swap(mRTView, ptr->mRTView);
			std::swap(mSRView, ptr->mSRView);
			std::swap(mUAV, ptr->mUAV);
			std::swap(mDesc, ptr->mDesc);
		}

	protected:
		virtual void initTexture()
		{
			bool unorderedAccess = ((mDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0);
			bool shaderRecource = ((mDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0);
			bool rendertarget = ((mDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0);
			bool depthstencil = ((mDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0);

			if (rendertarget && depthstencil)
				error("cannot support a texture with rtview and dsview");


			if (rendertarget)
				checkResult(getDevice()->CreateRenderTargetView(mTexture, nullptr, &mRTView));

			if (unorderedAccess)
			{
				checkResult(getDevice()->CreateUnorderedAccessView(mTexture, nullptr, &mUAV));
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

				D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
				ZeroMemory(&descDSV, sizeof(descDSV));
				descDSV.Format = DSVfmt;
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				descDSV.Texture2D.MipSlice = 0;
				checkResult(getDevice()->CreateDepthStencilView(mTexture, &descDSV, &mDSView));

				D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
				srvd.Format = SRVfmt;
				srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvd.Texture2D.MostDetailedMip = 0;
				srvd.Texture2D.MipLevels = 1;

				checkResult(getDevice()->CreateShaderResourceView(mTexture, &srvd, &mSRView));
			}
			else
			{
				if (depthstencil)
					checkResult(getDevice()->CreateDepthStencilView(mTexture, nullptr, &mDSView));

				if (shaderRecource)
					checkResult(getDevice()->CreateShaderResourceView(mTexture, nullptr, &mSRView));
			}
		}
	protected:
		T* mTexture;
		D mDesc;
	};

	class Texture2D :public TextureUnknown<ID3D11Texture2D, D3D11_TEXTURE2D_DESC>
	{
		using Parent = TextureUnknown<ID3D11Texture2D, D3D11_TEXTURE2D_DESC>;
	public:
		Texture2D(Renderer* r, ID3D11Texture2D* tex) :D3DObject(r), Parent(tex)
		{
			initTexture();
		}
	};

	class Texture3D :public TextureUnknown<ID3D11Texture3D, D3D11_TEXTURE3D_DESC>
	{
		using Parent = TextureUnknown<ID3D11Texture3D, D3D11_TEXTURE3D_DESC>;
	public:
		Texture3D(Renderer* r, ID3D11Texture3D* tex) :D3DObject(r), Parent(tex)
		{
			initTexture();
		}
	};

	class TextureCube :public TextureUnknown<ID3D11Texture2D, D3D11_TEXTURE2D_DESC>
	{
		using Parent = TextureUnknown<ID3D11Texture2D, D3D11_TEXTURE2D_DESC>;
	public:
		TextureCube(Renderer* r, ID3D11Texture2D* tex) :D3DObject(r), Parent(tex)
		{
			initTexture();
		}

	private:
		void initTexture()
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			desc.Format = mDesc.Format;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = mDesc.MipLevels;
			desc.TextureCube.MostDetailedMip = 0;

			checkResult(getDevice()->CreateShaderResourceView(mTexture, &desc, &mSRView));
		}
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
		Layout(Renderer* renderer, const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count);
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
		Font(Renderer* renderer, const std::wstring& font);
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
		Rasterizer(Renderer* renderer, const D3D11_RASTERIZER_DESC& desc);
		~Rasterizer();

		operator ID3D11RasterizerState*()const { return mRasterizer; }
	private:
		D3D11_RASTERIZER_DESC mDesc;
		ID3D11RasterizerState* mRasterizer;
	};


	class Profile :public NODefault, public D3DObject
	{
		friend class Renderer;
	public :
		using Ptr = std::weak_ptr<Profile>;

		class Timer
		{
		public:
			Timer(Profile* p):profile(p)
			{
				p->getContext()->End(p->mBegin);
			}

			~Timer()
			{
				profile->getContext()->End(profile->mEnd);
			}

			Profile* profile;
		};
	public:
		Profile(Renderer* r);
		~Profile();

		Timer count();
		float getElapsedTime()const { return mElapsedTime; }
	private:
		void getData(UINT64 frq);
	private:
		ID3D11Query* mBegin;
		ID3D11Query* mEnd;
		float mCachedTime = 0;
		int mNumCached = 0;
		float mElapsedTime = 0;
	};


	using CompiledData = Interface<ID3D10Blob>;
	using SharedCompiledData = CompiledData::Shared ;
	using PixelShader = Interface<ID3D11PixelShader>;
	using ComputeShader = Interface<ID3D11ComputeShader>;

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

	//void setTexture(const Texture::Ptr tex);
	//void setTextures(const std::vector<Texture::Ptr>& texs);
	//void setTexture(const RenderTarget::Ptr tex);
	//void setTextures(const std::vector<RenderTarget::Ptr>& texs);
	void setTexture(ShaderResource::Ptr srv);
	void setTextures(const std::vector<ShaderResource::Ptr>& srvs);
	template<class Container>
	void setTextures(const Container& c)
	{
		std::vector<ShaderResource::Ptr> list;
		list.reserve(c.size());
		for (auto& i : c)
			list.push_back(i);

		setTextures(list);
	}
	void removeShaderResourceViews();

	void setSampler(Sampler::Ptr s);
	void setSamplers(const std::vector<Sampler::Ptr>& ss);
	void removeSamplers();
	Sampler::Ptr getSampler(const std::string&name);
	void setRenderTarget(RenderTarget::Ptr rt, DepthStencil::Ptr ds = DepthStencil::Ptr());
	void setRenderTargets(const std::vector<RenderTarget::Ptr>& rts, DepthStencil::Ptr ds = DepthStencil::Ptr());
	void removeRenderTargets();
	void setVSConstantBuffers(const std::vector<Buffer::Ptr>& bs);
	void setPSConstantBuffers(const std::vector<Buffer::Ptr>& bs);

	void setRasterizer(Rasterizer::Ptr r);
	void setRasterizer(const D3D11_RASTERIZER_DESC& desc);

	void setDefaultRasterizer();

	Texture::Ptr getBackbuffer() { return mBackbuffer; }
	void setLayout(ID3D11InputLayout* layout);
	void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY pri);
	void setIndexBuffer(Buffer::Ptr b, DXGI_FORMAT format, int offset);
	void setVertexBuffer(Buffer::Ptr b, UINT stride, UINT offset);
	void setVertexBuffers(const std::vector<Buffer::Ptr>& b, const UINT* stride, const UINT* offset);
	void setViewport(const D3D11_VIEWPORT& vp);
	void setDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc, UINT stencil = 0);
	void setBlendState(const D3D11_BLEND_DESC& desc, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);
	void setDefaultBlendState();
	void setDefaultDepthStencilState();
	void setVertexShader(VertexShader::Weak shader);
	void setPixelShader(PixelShader::Weak shader);
	void setComputeShder(ComputeShader::Weak shader);


	Sampler::Ptr createSampler(const std::string& name, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV,
		D3D11_TEXTURE_ADDRESS_MODE addrW = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_FUNC cmpfunc = D3D11_COMPARISON_NEVER, float minlod = 0, float maxlod = D3D11_FLOAT32_MAX);
	Texture2D::Ptr createTexture(const std::string& filename);
	Texture2D::Ptr createTexture( const D3D11_TEXTURE2D_DESC& desc, const void* data = 0, size_t size = 0);
	Texture3D::Ptr createTexture3D(const D3D11_TEXTURE3D_DESC& desc, const void* data = 0, size_t size = 0);
	TextureCube::Ptr createTextureCube(const std::string& file);
	TextureCube::Ptr createTextureCube(const std::array<std::string,6>& file);


	Texture2D::Ptr createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	Buffer::Ptr createBuffer(int size, D3D11_BIND_FLAG bindflag, const D3D11_SUBRESOURCE_DATA* initialdata = NULL,D3D11_USAGE usage = D3D11_USAGE_DEFAULT, size_t CPUaccess = 0);
	Buffer::Ptr createRWBuffer(int size, int stride, DXGI_FORMAT format, size_t bindflag,  D3D11_USAGE usage = D3D11_USAGE_DEFAULT, size_t CPUaccess = 0);

	SharedCompiledData compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro = NULL);
	Effect::Ptr createEffect(void* data, size_t size);
	VertexShader::Weak createVertexShader(const void* data, size_t size);
	PixelShader::Weak createPixelShader(const void* data, size_t size);
	PixelShader::Weak createPixelShader(const std::string& file, const std::string& entry);

	ComputeShader::Weak createComputeShader(const void* data, size_t size);
	ComputeShader::Weak createComputeShader(const std::string& name, const std::string& entry);

	Layout::Ptr createLayout(const D3D11_INPUT_ELEMENT_DESC* descarray, size_t count);
	Font::Ptr createOrGetFont(const std::wstring& font);
	Rasterizer::Ptr createOrGetRasterizer(  const D3D11_RASTERIZER_DESC& desc);
	Texture2D::Ptr createDepthStencil(int width, int height, DXGI_FORMAT format, bool access = false);

	Profile::Ptr createProfile();

 private:
	static void checkResult(HRESULT hr);
	static void error(const std::string& err);

	void setConstantBuffersImpl(const std::vector<Buffer::Ptr>& bs, std::function<void(size_t, ID3D11Buffer**)> f);
private:
	int mWidth = 0;
	int mHeight = 0;

	ID3D11Device* mDevice;
	ID3D11DeviceContext* mContext;
	IDXGISwapChain* mSwapChain;
	D3D_FEATURE_LEVEL mFeatureLevel;
	ID3D11Query* mDisjoint;


	DepthStencil::Ptr mDefaultDepthStencil;
	Rasterizer::Ptr mRasterizer;
	Texture::Ptr mBackbuffer;

	size_t mSamplersState = 0;
	size_t mSRVState = 0;
	size_t mConstantState = 0;

	std::vector<std::shared_ptr<Buffer>> mBuffers;
	std::vector<std::shared_ptr<Effect>> mEffects;
	std::vector<std::shared_ptr<Layout>> mLayouts;

	std::vector<std::shared_ptr<Texture>> mTextures;
	std::unordered_map<std::string,std::shared_ptr<Sampler>> mSamplers;
	std::vector<VertexShader::Shared> mVSs;
	std::vector<PixelShader::Shared> mPSs;
	std::vector<ComputeShader::Shared> mCSs;
	std::vector<std::shared_ptr<Profile>> mProfiles;
	std::unordered_map<std::wstring, std::shared_ptr<Font>> mFonts;
	std::unordered_map<size_t, std::shared_ptr<Rasterizer>> mRasterizers;

	std::unordered_map<size_t, DepthStencilState::Shared> mDepthStencilStates;
	std::unordered_map<size_t, BlendState::Shared> mBlendStates;
};



#define PROFILE(x) auto _profile = x.lock()->count();