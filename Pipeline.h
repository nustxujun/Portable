#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Setting.h"

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
		Stage(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr set, Pipeline* p);
		virtual ~Stage();
		void update(Renderer::Texture::Ptr rt);
		virtual void render(Renderer::Texture::Ptr rt) = 0;
		virtual void onChanged(const std::string& key, const nlohmann::json::value_type& value)override {};

		Renderer::Ptr getRenderer()const { return mRenderer; }
		Scene::Ptr getScene()const { return mScene; }
		virtual const std::string& getName() const{ return mName; }

		void showCost();
	protected:
		Quad::Ptr mQuad;
		std::string mName = "Default Stage";

	private:
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
		Anonymous(Renderer::Ptr r, Scene::Ptr s,Setting::Ptr st, Pipeline* p, DrawCall drawcall) :Stage(r, s,st, p), mDrawCall(drawcall)
		{
			mName = "Anonymous";
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
		auto ptr = new T(mRenderer, mScene, mSetting,this, args...);
		mStages.emplace_back(Stage::Ptr(ptr));
	}

	void pushStage(Anonymous::DrawCall dc);

	void render();
	void setFrameBuffer(Renderer::Texture::Ptr rt);
	Setting::Ptr getSetting()const { return mSetting; }

	void onChanged(const std::string& key, const nlohmann::json::value_type& value) override {};

private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	std::vector<Stage::Ptr> mStages;
	Renderer::Texture::Ptr mFrameBuffer;
	Setting::Ptr mSetting;
	Renderer::Profile::Ptr mProfile;
};
