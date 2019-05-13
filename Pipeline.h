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
		void addSharedRT(const std::string& name, Renderer::RenderTarget::Ptr rt);
		void addSharedDS(const std::string& name, Renderer::DepthStencil::Ptr rt);
		Renderer::RenderTarget::Ptr getSharedRT(const std::string& name);
		Renderer::DepthStencil::Ptr getSharedDS(const std::string& name);

	protected:
		Quad::Ptr mQuad;
	private:
		Renderer::Ptr mRenderer;
		Scene::Ptr mScene;
		Pipeline* mPipeline;
	};
public:
	Pipeline(Renderer::Ptr r, Scene::Ptr s);

	template<class T, class ... Args>
	void pushStage(Args ... args)
	{
		auto ptr = new T(mRenderer, mScene,this, args...);
		mStages.emplace_back(Stage::Ptr(ptr));
	}

	void render();
private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	std::vector<Stage::Ptr> mStages;
	std::unordered_map<std::string, Renderer::RenderTarget::Ptr> mSharedRenderTargets;
	std::unordered_map<std::string, Renderer::DepthStencil::Ptr> mSharedDepthStencils;
};
