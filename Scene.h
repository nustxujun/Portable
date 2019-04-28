#pragma once
#include <memory>
#include <string>
#include <functional>
#include <map>
#include "Mesh.h"


class Scene
{
public:
	using Parameters = std::map<std::string, std::string>;
	using Ptr = std::shared_ptr<Scene>;

	class Entity
	{
	public:
		using Ptr = std::shared_ptr<Entity>;
	public:
		Entity(const Parameters& params,Renderer::Ptr r);
		~Entity();
		Mesh::Ptr getMesh() { return mMesh; };
	private:
		Mesh::Ptr mMesh;
	};
public:
	Scene(Renderer::Ptr renderer);
	~Scene();

	Entity::Ptr createEntity(const std::string& name, const Parameters& params);

	void visit(std::function<void(Entity::Ptr)> callback);

private:
	Renderer::Ptr mRenderer;
	std::map<std::string, Entity::Ptr> mEntities;
};