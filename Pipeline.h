#pragma once

#include "renderer.h"
#include "Scene.h"
#include "Quad.h"
class Pipeline
{
public:
	class Stage
	{
	public:
		using Ptr = std::shared_ptr<Stage>;
	public:
		Stage(Renderer::Ptr r, Scene::Ptr s);
		virtual ~Stage();

		virtual void render(Renderer::RenderTarget::Ptr rt) = 0;

		Renderer::Ptr getRenderer()const { return mRenderer; }
		Scene::Ptr getScene()const { return mScene; }

	protected:
		Quad::Ptr mQuad;
	private:
		Renderer::Ptr mRenderer;
		Scene::Ptr mScene;
	};
public:
	Pipeline(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q);

	template<class T, class ... Args>
	void pushStage(Args ... args)
	{
		auto ptr = new T(mRenderer, mScene, args...);
		mStages.emplace_back(Stage::Ptr(ptr));
	}

	void render();
private:
	Renderer::Ptr mRenderer;
	Scene::Ptr mScene;
	std::vector<Stage::Ptr> mStages;

};
