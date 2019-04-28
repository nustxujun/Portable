#include "Scene.h"

Scene::Scene(Renderer::Ptr r) : mRenderer(r)
{
}

Scene::~Scene()
{
}

Scene::Entity::Ptr Scene::createEntity(const std::string & name, const Parameters& params)
{
	auto ret = mEntities.find(name);
	if (ret != mEntities.end())
		return ret->second;

	Entity::Ptr e(new Entity(params, mRenderer));
	mEntities.emplace(name, e);

	return e;
}

void Scene::visit(std::function<void(Entity::Ptr)> callback)
{
	for (auto i : mEntities)
	{
		callback(i.second);
	}
}

Scene::Entity::Entity(const Parameters & params, Renderer::Ptr r)
{
	auto endi = params.end();
	auto ret = params.find("file");
	if (ret != endi)
	{
		Mesh* mesh = new Mesh(ret->second, r);
		mMesh = decltype(mMesh)(mesh);
	}
}

Scene::Entity::~Entity()
{
}
