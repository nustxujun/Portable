#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Setting.h"

#define VIEW_METHOD(x)\
	 void add##x(const std::string& name, Renderer::x::Ptr ptr){mPipeline->add##x(name, ptr);}\
	 Renderer::x::Ptr get##x(const std::string& name){ return mPipeline->get##x(name);}



class Pipeline:public Setting::Modifier
{
	friend class Stage;
public:
	class Stage: public Setting::Modifier
	{
		friend class Pipeline;
	public:
		using Ptr = std::shared_ptr<Stage>;
	public:
		Stage(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q,Setting::Ptr set, Pipeline* p);
		virtual ~Stage();
		void update(Renderer::Texture::Ptr rt);
		virtual void render(Renderer::Texture::Ptr rt) = 0;
		virtual void onChanged(const std::string& key, const nlohmann::json::value_type& value)override {};
		virtual void init() {};

		Renderer::Ptr getRenderer()const { return mRenderer; }
		Scene::Ptr getScene()const { return mScene; }
		Quad::Ptr getQuad()const { return mQuad; }
		virtual const std::string& getName() const{ return mName; }

		void showCost();


		VIEW_METHOD(ShaderResource);
		VIEW_METHOD(RenderTarget);
		VIEW_METHOD(DepthStencil);
		VIEW_METHOD(UnorderedAccess);
		VIEW_METHOD(Buffer);
	protected:
		std::string mName = "Default Stage";
	private:
		Quad::Ptr mQuad;
		Renderer::Ptr mRenderer;
		Scene::Ptr mScene;
		Pipeline* mPipeline;
		Renderer::Profile::Ptr mProfile;

		size_t mCachedTimes = 0;
		size_t mCachedNumFrames = 0;
		size_t mFPS = 0;
	};

	class Anonymous : public Stage
	{
	public:
		using DrawCall = std::function<void(Renderer::Texture::Ptr)>;
	public:
		Anonymous(Renderer::Ptr r, Scene::Ptr s,Quad::Ptr q,Setting::Ptr st, Pipeline* p, const std::string& name, DrawCall drawcall) :Stage(r, s,q,st, p),mDrawCall(drawcall)
		{
			mName = name;
		}

		void render(Renderer::Texture::Ptr rt) override final
		{
			mDrawCall(rt);
		}

	private:
		DrawCall mDrawCall;
	};

public:
	Pipeline(Renderer::Ptr r, Scene::Ptr s);

	template<class T, class ... Args>
	void pushStage(Args ... args)
	{
		auto ptr = new T(mRenderer, mScene, mQuad,mSetting,this);
		ptr->init( args...);
		mStages.emplace_back(Stage::Ptr(ptr));
	}

	void pushStage(const std::string& name, Anonymous::DrawCall dc);

	void render();
	void setFrameBuffer(Renderer::Texture::Ptr rt);
	Setting::Ptr getSetting()const { return mSetting; }

	void onChanged(const std::string& key, const nlohmann::json::value_type& value) override {};

	void addShaderResource(const std::string& name, Renderer::ShaderResource::Ptr ptr);
	void addRenderTarget(const std::string& name, Renderer::RenderTarget::Ptr ptr);
	void addDepthStencil(const std::string& name, Renderer::DepthStencil::Ptr ptr);
	void addUnorderedAccess(const std::string& name, Renderer::UnorderedAccess::Ptr ptr);
	void addBuffer(const std::string& name, Renderer::Buffer::Ptr ptr);


	Renderer::ShaderResource::Ptr getShaderResource(const std::string& name);
	Renderer::RenderTarget::Ptr getRenderTarget(const std::string& name);
	Renderer::DepthStencil::Ptr getDepthStencil(const std::string& name);
	Renderer::UnorderedAccess::Ptr getUnorderedAccess(const std::string& name);
	Renderer::Buffer::Ptr getBuffer(const std::string& name);

private:
	static void report(const std::string& msg);
private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	Quad::Ptr mQuad;
	std::vector<Stage::Ptr> mStages;
	Renderer::Texture::Ptr mFrameBuffer;
	Setting::Ptr mSetting;
	Renderer::Profile::Ptr mProfile;

	std::unordered_map<std::string, Renderer::ShaderResource::Ptr> mShaderResources;
	std::unordered_map<std::string, Renderer::RenderTarget::Ptr> mRenderTargets;
	std::unordered_map<std::string, Renderer::DepthStencil::Ptr> mDepthStencils;
	std::unordered_map<std::string, Renderer::UnorderedAccess::Ptr> mUnorderedAccesses;
	std::unordered_map<std::string, Renderer::Buffer::Ptr> mBuffers;

};
