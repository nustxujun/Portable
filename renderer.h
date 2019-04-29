#pragma once

#include <algorithm>
#include "d3dx11effect.h"
#include <D3D11.h>
#include <D3DX11.h>
#include <memory>
#include <set>
#include <string>
#include <map>
#include <functional>
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
		D3DObject(ID3D11Device* device): mDevice(device){}
	protected:
		ID3D11Device* getDevice()const { return mDevice; }
	private:
		ID3D11Device* mDevice;
	};
public:
	template<class T>
	class Interface final : public NODefault
	{
	public:
		using Ptr = std::shared_ptr<Interface<T>>;

		Interface(T* t) : mInterface(t) {}
		~Interface() { if (mInterface) mInterface->Release(); }

		operator T*() { return mInterface; }
	private:
		T* mInterface;
	};

	class RenderTarget final : public NODefault
	{
	public:
		using Ptr = std::weak_ptr<RenderTarget>;
	public:
		RenderTarget(ID3D11Texture2D* t, ID3D11RenderTargetView* rtv, ID3D11ShaderResourceView* srv);
		~RenderTarget();
		operator ID3D11RenderTargetView*() { return mRTView; }
		
		ID3D11Texture2D* getTexture() { return mTexture; }
		ID3D11ShaderResourceView* getShaderResourceView() { return mSRView; }
	private:
		ID3D11Texture2D* mTexture;
		ID3D11RenderTargetView* mRTView;
		ID3D11ShaderResourceView* mSRView;
	};

	class Buffer final : public NODefault
	{
	public: 
		using Ptr = std::weak_ptr<Buffer>;
	public:
		Buffer(ID3D11Buffer* b);
		~Buffer();

		operator ID3D11Buffer* ()const {return mBuffer;}
	private:
		ID3D11Buffer* mBuffer;
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
		std::map<std::string, ID3DX11EffectTechnique*> mTechs;
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

	class Layout final : public NODefault, public D3DObject
	{
	public:
		using Ptr = std::weak_ptr<Layout>;
	public:
		Layout(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC * descarray, size_t count);
		~Layout(){}

		ID3D11InputLayout* bind(const char* data, size_t size);
	private:
		std::map<size_t, Interface<ID3D11InputLayout>::Ptr> mBindedLayouts;
		std::vector<D3D11_INPUT_ELEMENT_DESC> mElements;
	};

	using CompiledData = Interface<ID3D10Blob>;
	using SharedCompiledData = std::shared_ptr<CompiledData> ;
public:
	void init(HWND win, int width, int height);
	void uninit();

	int getWidth()const { return mWidth; }
	int getHeight()const { return mHeight; }

	ID3D11DeviceContext* getContext() { return mContext; };

	void present();
	void clearDepth(float depth);
	void clearStencil(int stencil);
	void clearRenderTarget(RenderTarget::Ptr rt, const float color[4]);

	void setTexture(const Texture::Ptr tex);
	void setTextures(const std::vector<Texture::Ptr>& texs);

	void setRenderTarget(RenderTarget::Ptr rt);
	void setRenderTargets(std::vector<RenderTarget::Ptr>& rts);
	RenderTarget::Ptr getBackbuffer() { return mBackbuffer; }
	void setLayout(ID3D11InputLayout* layout);
	void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY pri);
	void setIndexBuffer(Buffer::Ptr b, DXGI_FORMAT format, int offset);
	void setVertexBuffer(Buffer::Ptr b, size_t stride, size_t offset);
	void setVertexBuffers( std::vector<Buffer::Ptr>& b, const size_t* stride, const size_t* offset);

	Sampler::Ptr createSampler(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addrU, D3D11_TEXTURE_ADDRESS_MODE addrV,
		D3D11_TEXTURE_ADDRESS_MODE addrW = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_FUNC cmpfunc = D3D11_COMPARISON_NEVER, float minlod = 0, float maxlod = D3D11_FLOAT32_MAX);
	Texture::Ptr createTexture(const std::string& filename);
	RenderTarget::Ptr createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	Buffer::Ptr createBuffer(int size, D3D11_BIND_FLAG flag, const D3D11_SUBRESOURCE_DATA* initialdata = NULL,D3D11_USAGE usage = D3D11_USAGE_DEFAULT, bool CPUaccess = false);
	SharedCompiledData compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro = NULL);
	Effect::Ptr createEffect(void* data, size_t size);
	Layout::Ptr createLayout(const D3D11_INPUT_ELEMENT_DESC* descarray, size_t count);

private:
	void initRenderTargets();
	static void checkResult(HRESULT hr);

private:
	int mWidth = 0;
	int mHeight = 0;

	ID3D11Device* mDevice;
	ID3D11DeviceContext* mContext;
	IDXGISwapChain* mSwapChain;
	D3D_FEATURE_LEVEL mFeatureLevel;
	ID3D11Texture2D* mDepthStencil;
	ID3D11DepthStencilView* mDepthStencilView;
	ID3D11RasterizerState* mRasterizer;

	RenderTarget::Ptr mBackbuffer;

	std::set<std::shared_ptr<RenderTarget>> mRenderTargets;
	std::set<std::shared_ptr<Buffer>> mBuffers;
	std::set<std::shared_ptr<Effect>> mEffects;
	std::set<std::shared_ptr<Layout>> mLayouts;
	std::map<std::string,std::shared_ptr<Texture>> mTextures;
	std::set<std::shared_ptr<Sampler>> mSamplers;

};



