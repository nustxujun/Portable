#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"
#include "Setting.h"

class Pipeline
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

		virtual void render(Renderer::RenderTarget::Ptr rt) = 0;
		virtual void onChanged(const std::string& key, const nlohmann::json::value_type& value)override {};

		Renderer::Ptr getRenderer()const { return mRenderer; }
		Scene::Ptr getScene()const { return mScene; }
	protected:
		Quad::Ptr mQuad;
	private:
		Renderer::Ptr mRenderer;
		Scene::Ptr mScene;
		Pipeline* mPipeline;
	};

	class Anonymous : public Stage
	{
	public:
		using DrawCall = std::function<void(Renderer::RenderTarget::Ptr)>;
	public:
		Anonymous(Renderer::Ptr r, Scene::Ptr s,Setting::Ptr st, Pipeline* p, DrawCall drawcall) :Stage(r, s,st, p), mDrawCall(drawcall)
		{
		}

		void render(Renderer::RenderTarget::Ptr rt) override final
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
	void setFrameBuffer(Renderer::RenderTarget::Ptr rt);
	Setting::Ptr getSetting()const { return mSetting; }
private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	std::vector<Stage::Ptr> mStages;
	Renderer::RenderTarget::Ptr mFrameBuffer;
	Setting::Ptr mSetting;
};
