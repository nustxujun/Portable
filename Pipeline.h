#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"
class Pipeline
{
	friend class Stage;
public:
	class Stage
	{
		friend class Pipeline;
	public:
		using Ptr = std::shared_ptr<Stage>;
	public:
		Stage(Renderer::Ptr r, Scene::Ptr s, Pipeline* p);
		virtual ~Stage();

		virtual void render(Renderer::RenderTarget::Ptr rt) = 0;

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
		Anonymous(Renderer::Ptr r, Scene::Ptr s, Pipeline* p, DrawCall drawcall) :Stage(r, s, p), mDrawCall(drawcall)
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
		auto ptr = new T(mRenderer, mScene,this, args...);
		mStages.emplace_back(Stage::Ptr(ptr));
	}

	void pushStage(Anonymous::DrawCall dc);

	void render();
	void setFrameBuffer(Renderer::RenderTarget::Ptr rt);
private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	std::vector<Stage::Ptr> mStages;
	Renderer::RenderTarget::Ptr mFrameBuffer;
};
