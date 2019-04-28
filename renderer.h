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
public:
	template<class T>
	class Interface final : public NODefault
	{
	public:
		using Ptr = std::weak_ptr<Interface<T>>;

		Interface(T* t) : mInterface(t) {}
		~Interface() { mInterface->Release(); }

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

	class Effect final: public NODefault
	{
	public:
		using Ptr = std::weak_ptr<Effect>;
	public:
		Effect(ID3DX11Effect* eff);
		~Effect();
		void render(ID3D11DeviceContext* context, std::function<void(ID3DX11EffectTechnique*, int)> prepare, std::function<void(ID3D11DeviceContext* context)> draw);

		void setTech(const std::string& name);
		ID3DX11EffectTechnique* getTech(const std::string& name);
		operator ID3DX11Effect*() { return mEffect; }

		ID3DX11EffectVariable* getVariable(const std::string& name);
		ID3DX11EffectConstantBuffer * getConstantBuffer(const std::string& name);

	private:
		ID3DX11Effect* mEffect;
		std::string mSelectedTech;
		std::map<std::string, ID3DX11EffectTechnique*> mTechs;
	};

	using Layout = Interface<ID3D11InputLayout>;
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
	void setRenderTarget(RenderTarget::Ptr rt);
	void setRenderTargets(std::vector<RenderTarget::Ptr>& rts);
	RenderTarget::Ptr getBackbuffer() { return mBackbuffer; }
	void setLayout(Layout::Ptr layout);
	void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY pri);
	void setIndexBuffer(Buffer::Ptr b, DXGI_FORMAT format, int offset);
	void setVertexBuffer(Buffer::Ptr b, size_t stride, size_t offset);
	void setVertexBuffers( std::vector<Buffer::Ptr>& b, const size_t* stride, const size_t* offset);

	RenderTarget::Ptr createRenderTarget(int width, int height, DXGI_FORMAT format, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	Buffer::Ptr createBuffer(int size, D3D11_BIND_FLAG flag, const D3D11_SUBRESOURCE_DATA* initialdata = NULL,D3D11_USAGE usage = D3D11_USAGE_DEFAULT, bool CPUaccess = false);
	SharedCompiledData compileFile(const std::string& filename, const std::string& entryPoint, const std::string& shaderModel, const D3D10_SHADER_MACRO* macro = NULL);
	Effect::Ptr createEffect(void* data, size_t size);
	Layout::Ptr createLayout(const D3D11_INPUT_ELEMENT_DESC* descarray, size_t count, void* data, size_t size);

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
	std::vector<ID3D11RenderTargetView*> mCurrentTargets;



	std::set<std::shared_ptr<RenderTarget>> mRenderTargets;
	std::set<std::shared_ptr<Buffer>> mBuffers;
	std::set<std::shared_ptr<Effect>> mEffects;
	std::set<std::shared_ptr<Layout>> mLayouts;

};



