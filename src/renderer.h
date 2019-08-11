#pragma once

#include <algorithm>
#include "Common.h"
#include "SpriteFont.h"
#include "SimpleMath.h"
#include <vector>
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

	template<class T>
	class WeakWrapper
	{
	public:
		template<class U>
		WeakWrapper(std::weak_ptr<U> ptr) { mInstance = ptr; }
		template<class U>
		WeakWrapper(std::shared_ptr<U> ptr) { mInstance = ptr; }
		template<class U>
		WeakWrapper(WeakWrapper<U> ptr) { mInstance = ptr.get(); }

		WeakWrapper() {}
		WeakWrapper(std::shared_ptr<T> ptr) { mInstance = ptr; }
		WeakWrapper(std::weak_ptr<T> ptr) { mInstance = ptr; }
		operator std::weak_ptr<T>() const { return mInstance; }

		std::shared_ptr<T> operator->() const { return mInstance.lock(); }

		bool expired()const { return mInstance.expired(); }
		std::shared_ptr<T> lock()const { return mInstance.lock(); }
		std::weak_ptr<T> get()const { return mInstance; }
	private:
		std::weak_ptr<T> mInstance;
	};

	template<class T>
	class View
	{
	public:
		using Ptr = WeakWrapper<View>;
		View() {}
		~View() {
			for (auto&i : mViews)
				i->Release();
		}
		void addView(T* v) { mViews.push_back(v); }
		T* getView(size_t index = 0) { 
			if (mViews.empty())
				return nullptr;
			return mViews[index]; 
		}
		size_t getNumViews()const { return mViews.size(); }
		operator T*()const { return mViews[0]; }
		void swap(Ptr ptr) {
			std::swap(mViews, ptr->mViews);
		}
	private:
		std::vector<T*> mViews;
	};

	using RenderTarget = View<ID3D11RenderTargetView>;
	using ShaderResource = View<ID3D11ShaderResourceView>;
	using UnorderedAccess = View<ID3D11UnorderedAccessView>;
	using DepthStencil = View<ID3D11DepthStencilView>;

	class Buffer :public D3DObject, public ShaderResource, public UnorderedAccess
	{
	public: 
		using Ptr = WeakWrapper<Buffer>;
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
		void map();
		void unmap();
		void write(const void*data, size_t size);
		void skip(size_t size);
		template<class T>
		void write(const T& data)
		{
			write(&data, sizeof(data));
		}



		operator ID3D11Buffer* ()const {return mBuffer;}
		ID3D11Buffer* getBuffer()const { return mBuffer; }


	private:
		ID3D11Buffer* mBuffer;
		D3D11_BUFFER_DESC mDesc;
		std::vector<char> mCache;
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
		ID3DX11EffectTechnique* mSelectedTech;
		std::unordered_map<std::string, ID3DX11EffectTechnique*> mTechs;
	};

	class Texture : public D3DObject,public RenderTarget, public ShaderResource, public UnorderedAccess, public DepthStencil
	{
	public:
		using Ptr = WeakWrapper<Texture>;
	public:
		Texture(Renderer* r) :D3DObject(r) {}
		virtual ~Texture();

	protected:
	};

	template<class T, class D>
	class TextureUnknown : public Texture
	{
	public:
		using Ptr = WeakWrapper<TextureUnknown>;
	public:
		TextureUnknown(Renderer* r,T* tex ):Texture(r)
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
			ShaderResource::swap(t);
			RenderTarget::swap(t);
			UnorderedAccess::swap(t);
			DepthStencil::swap(t);
			std::swap(mDesc, ptr->mDesc);
		}

	protected:
		virtual void initTexture() = 0;
	protected:
		T* mTexture;
		D mDesc;
	};

	class Texture2D :public TextureUnknown<ID3D11Texture2D, D3D11_TEXTURE2D_DESC>
	{
		using Parent = TextureUnknown<ID3D11Texture2D, D3D11_TEXTURE2D_DESC>;
	public:
		using Ptr = WeakWrapper<Texture2D>;

		Texture2D(Renderer* r, ID3D11Texture2D* tex) : Parent(r,tex)
		{
			initTexture();
		}

		void initTexture();

		void blit(const void* data, size_t size, UINT index = 0);
	};

	class Texture3D :public TextureUnknown<ID3D11Texture3D, D3D11_TEXTURE3D_DESC>
	{
		using Parent = TextureUnknown<ID3D11Texture3D, D3D11_TEXTURE3D_DESC>;
	public:
		Texture3D(Renderer* r, ID3D11Texture3D* tex) : Parent(r,tex)
		{
			initTexture();
		}

		void initTexture();
	};

	class TemporaryRT
	{
	public:
		using Ptr = std::unique_ptr<TemporaryRT>;
		static Ptr create(std::shared_ptr<bool> ref, Texture2D::Ptr tex)
		{
			return Ptr(new TemporaryRT(ref, tex));
		}

		TemporaryRT(std::shared_ptr<bool> ref, Texture2D::Ptr tex):
			mRef(ref),mRT(tex)
		{
			*ref = true;
		}

		operator Texture2D::Ptr()const
		{
			return mRT;
		}

		Texture2D::Ptr operator->()const
		{
			return mRT;
		}

		Texture2D::Ptr get()const
		{
			return mRT;
		}

		~TemporaryRT()
		{
			*mRef = false;
		}
	private:
		std::shared_ptr<bool> mRef;
		Texture2D::Ptr mRT;
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

		void setRenderTarget(Texture2D::Ptr rt) { mTarget = rt; }
	private:
		std::unique_ptr<DirectX::SpriteFont> mFont;
		std::unique_ptr<DirectX::SpriteBatch> mBatch;
		float x = 0;
		float y = 0;
		Texture2D::Ptr mTarget;
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
			Timer(Profile* p);

			~Timer();

			Profile* profile;
		};
	public:
		Profile(Renderer* r);
		~Profile();

		Timer count();
		float getGPUElapsedTime()const { return mGPUElapsedTime; }
		float getCPUElapsedTime()const { return mCPUElapsedTime; }

		void begin();
		void end();

		std::string toString()const;
	private:
		void getData(UINT64 frq);
	private:
		ID3D11Query* mBegin;
		ID3D11Query* mEnd;
		size_t mCachedTime = 0;
		int mNumCached = 0;
		float mGPUElapsedTime = 0;
		float mCPUElapsedTime = 0;
		size_t mCPUTimer = 0;
	};

	class CompiledData
	{
	public:
		using Ptr = std::shared_ptr<CompiledData>;
		CompiledData(size_t h , ID3D10Blob* b):blob(b),hash(h)
		{

		}
		CompiledData(size_t h, const std::vector<char>& buffer):mBuffer(buffer),hash(h)
		{

		}
		~CompiledData()
		{
			if (blob)
				blob->Release();
		}

		void* GetBufferPointer()
		{
			if (blob)
				return blob->GetBufferPointer();
			else
				return mBuffer.data();
		}

		size_t GetBufferSize()
		{
			if (blob)
				return blob->GetBufferSize();
			else
				return mBuffer.size();
		}

		size_t getHash() { return hash; }

	private:
		size_t hash;
		ID3D10Blob* blob = nullptr;
		std::vector<char> mBuffer;
	};


	//using CompiledData = Interface<ID3D10Blob>;
	using SharedCompiledData = CompiledData::Ptr;
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
	Texture2D::Ptr getBackbuffer() { return mBackbuffer; }
	Texture2D::Ptr getDefaultDepthStencil() { return mDefaultDepthStencil; };

	void present();
	void clearDepth(DepthStencil::Ptr rt, float depth);
	void clearStencil(DepthStencil::Ptr rt, UINT stencil);
	void clearDepthStencil(DepthStencil::Ptr rt,float depth, UINT stencil = 0);
	void clearRenderTarget(RenderTarget::Ptr rt, const std::array<float,4>& color, size_t index = -1);
	void clearUnorderedAccess(UnorderedAccess::Ptr rt, const std::array<float, 4>& value, size_t index = -1);
	void clearUnorderedAccess(UnorderedAccess::Ptr rt, const std::array<UINT, 4>& value, size_t index = -1);


	//void setTexture(const Texture::Ptr tex);
	//void setTextures(const std::vector<Texture::Ptr>& texs);
	//void setTexture(const RenderTarget::Ptr tex);
	//void setTextures(const std::vector<RenderTarget::Ptr>& texs);
	void setTexture(ShaderResource::Ptr srv);
	void setTextures(const std::vector<ShaderResource::Ptr>& srvs);
	template<class T>
	void setTextures(const std::vector<T>& c)
	{
		std::vector<ShaderResource::Ptr> list(c.size());
		
		for (size_t i = 0; i < c.size(); ++i)
			list[i] = c[i];
		setTextures(list);
	}
	void removeShaderResourceViews();

	void setSampler(Sampler::Ptr s);
	void setSamplers(const std::vector<Sampler::Ptr>& ss);
	void removeSamplers();
	Sampler::Ptr getSampler(const std::string&name);
	void setRenderTarget(RenderTarget::Ptr rt, DepthStencil::Ptr ds = {});
	void setRenderTarget(ID3D11RenderTargetView* rt, DepthStencil::Ptr ds = {});

	void setRenderTargets(const std::vector<RenderTarget::Ptr>& rts, DepthStencil::Ptr ds = {});
	void setRenderTargets(ID3D11RenderTargetView** rts, size_t size, DepthStencil::Ptr ds = {});

	void removeRenderTargets();
	void setVSConstantBuffers(const std::vector<Buffer::Ptr>& bs);
	void setPSConstantBuffers(const std::vector<Buffer::Ptr>& bs);

	void setRasterizer(Rasterizer::Ptr r);
	void setRasterizer(const D3D11_RASTERIZER_DESC& desc);

	void setDefaultRasterizer();

	void setLayout(ID3D11InputLayout* layout);
	void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY pri);
	void setIndexBuffer(Buffer::Ptr b, DXGI_FORMAT format, int offset);
	void setVertexBuffer(Buffer::Ptr b, UINT stride, UINT offset);
	void setVertexBuffers(const std::vector<Buffer::Ptr>& b, const UINT* stride, const UINT* offset);
	void setViewport(const D3D11_VIEWPORT& vp);
	void setDepthStencilState(const std::string& name, UINT stencil = 0);
	void setDepthStencilState(DepthStencilState::Weak dss, UINT stencil = 0);

	//void setBlendState(const D3D11_BLEND_DESC& desc, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);
	void setBlendState(const std::string& name, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);
	void setBlendState(BlendState::Weak blend, const std::array<float, 4>& factor = { 1,1,1,1 }, size_t mask = 0xffffffff);

	void setDefaultBlendState();
	void setDefaultDepthStencilState();
	void setVertexShader(VertexShader::Weak shader);
	void setPixelShader(PixelShader::Weak shader);
	void setComputeShder(ComputeShader::Weak shader);

	BlendState::Weak createBlendState(const std::string& name, const D3D11_BLEND_DESC& desc);
	Sampler::Ptr createSampler(const std::string& name, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV,
		D3D11_TEXTURE_ADDRESS_MODE addrW = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_FUNC cmpfunc = D3D11_COMPARISON_NEVER, float minlod = 0, float maxlod = D3D11_FLOAT32_MAX);
	Texture2D::Ptr createTexture(const std::string& filename, UINT miplevels = 0);
	Texture2D::Ptr createTexture( const D3D11_TEXTURE2D_DESC& desc);
	Texture3D::Ptr createTexture3D(const D3D11_TEXTURE3D_DESC& desc, const void* data = 0, size_t size = 0);
	Texture2D::Ptr createTextureCube(const std::string& file);
	Texture2D::Ptr createTextureCube(const std::array<std::string, 6>& file);
	void destroyTexture(Texture::Ptr tex);

	Texture2D::Ptr createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	TemporaryRT::Ptr createTemporaryRT(const D3D11_TEXTURE2D_DESC& desc);
	void destroyTemporaryRT(TemporaryRT::Ptr temp);
	
	Buffer::Ptr createBuffer(int size, D3D11_BIND_FLAG bindflag, const D3D11_SUBRESOURCE_DATA* initialdata = NULL,D3D11_USAGE usage = D3D11_USAGE_DEFAULT, size_t CPUaccess = 0);
	Buffer::Ptr createRWBuffer(int size, int stride, DXGI_FORMAT format, size_t bindflag,  D3D11_USAGE usage = D3D11_USAGE_DEFAULT, size_t CPUaccess = 0);
	Buffer::Ptr createConstantBuffer(int size, void* data = 0, size_t datasize = 0);
	SharedCompiledData compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro = NULL);
	Effect::Ptr createEffect(const std::string& file, const D3D10_SHADER_MACRO* macro = NULL);
	Effect::Ptr createEffect(void* data, size_t size);
	VertexShader::Weak createVertexShader(const void* data, size_t size);
	VertexShader::Weak createVertexShader(const std::string& file, const std::string& entry = "main", const D3D10_SHADER_MACRO* macro = NULL);
	PixelShader::Weak createPixelShader(const void* data, size_t size);
	PixelShader::Weak createPixelShader(const std::string& file, const std::string& entry = "main", const D3D10_SHADER_MACRO* macro = NULL);

	ComputeShader::Weak createComputeShader(const void* data, size_t size);
	ComputeShader::Weak createComputeShader(const std::string& name, const std::string& entry);

	Layout::Ptr createLayout(const D3D11_INPUT_ELEMENT_DESC* descarray, size_t count);
	Font::Ptr createOrGetFont(const std::wstring& font);
	Rasterizer::Ptr createOrGetRasterizer(  const D3D11_RASTERIZER_DESC& desc);
	Texture2D::Ptr createDepthStencil(int width, int height, DXGI_FORMAT format, bool access = false);
	DepthStencilState::Weak createDepthStencilState(const std::string& name, const D3D11_DEPTH_STENCIL_DESC& desc);

	Profile::Ptr createProfile();

 private:
	void initCompiledShaders();
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


	Texture2D::Ptr mDefaultDepthStencil;
	Rasterizer::Ptr mRasterizer;
	Texture2D::Ptr mBackbuffer;

	size_t mSamplersState = 0;
	size_t mSRVState = 0;
	size_t mConstantState = 0;

	std::vector<std::shared_ptr<Buffer>> mBuffers;
	std::vector<std::shared_ptr<Effect>> mEffects;
	std::vector<std::shared_ptr<Layout>> mLayouts;

	std::vector<std::shared_ptr<Texture>> mTextures;
	std::unordered_map<size_t, std::vector<std::pair<std::shared_ptr<bool>, Texture2D::Ptr>>> mTemporaryRTs;
	std::unordered_map<std::string, Texture2D::Ptr> mTextureMap;
	std::unordered_map<std::string,std::shared_ptr<Sampler>> mSamplers;
	std::unordered_map<size_t,VertexShader::Shared> mVSs;
	std::unordered_map<size_t, PixelShader::Shared> mPSs;
	std::unordered_map<size_t,ComputeShader::Shared> mCSs;
	std::unordered_map<size_t, SharedCompiledData> mCompiledShader;


	std::vector<std::shared_ptr<Profile>> mProfiles;
	std::unordered_map<std::wstring, std::shared_ptr<Font>> mFonts;
	std::unordered_map<size_t, std::shared_ptr<Rasterizer>> mRasterizers;

	std::unordered_map<std::string, DepthStencilState::Shared> mDepthStencilStates;
	std::unordered_map<std::string, BlendState::Shared> mBlendStates;
};



#define PROFILE(x) auto _profile = x.lock()->count();