#pragma once
#include "Common.h"
#include "Mesh.h"
#include <D3DX10math.h>

class Scene
{
public:
	using Ptr = std::shared_ptr<Scene>;
	class Entity;
	class Node
	{
		friend class Scene;
	public:
		using Ptr = std::shared_ptr<Node>;
	public:
		Node() :mPosition(0, 0, 0), mOrientation(0, 0, 0, 1) {};

		const D3DXMATRIX& getTransformation();

		template<class ... Args>
		void setPosition(const Args& ... args) { dirty(); mPosition = { args... }; }

		template<class ... Args>
		void setOrientation(const Args& ... args) { dirty(); mOrientation = { args... }; }

		void addChild(Ptr p) { mChildren.insert(p); p->mParent = this; p->dirty(); }
		void removeChild(Ptr p) { mChildren.erase(p); p->mParent = nullptr; p->dirty(); }
	private:
		void dirty();

	private:
		Node* mParent;
		std::set<Ptr> mChildren;
		std::shared_ptr<Entity> mEntity;
		D3DXVECTOR3 mPosition;
		D3DXQUATERNION mOrientation;
		D3DXMATRIX mTranformation;
		bool mDirty = true;
	};

	class Entity
	{
		friend class Scene;
	public:
		using Ptr = std::shared_ptr<Entity>;
	public:
		Entity();
		~Entity();

		virtual void visitRenderable(std::function<void(Renderable)>) = 0;
		void attach(Node::Ptr n) { n->addChild(mNode); };
		void detach() { if (!mNode->mParent) return; mNode->mParent->removeChild(mNode); }

		Node::Ptr getNode() { return mNode; }
	private:
		Node::Ptr mNode;
	};

	class Model : public Entity
	{
	public:
		using Ptr = std::shared_ptr<Model>;
	public:
		Model(const Parameters& params, Renderer::Ptr r);
		~Model();
		Mesh::Ptr getMesh() { return mMesh; };
		virtual void visitRenderable(std::function<void(Renderable)>) override;

	private:
		Mesh::Ptr mMesh;
	};

	class Camera: public Entity
	{
	public:
		using Ptr = std::shared_ptr<Camera>;
	public:
		Camera();
		~Camera();

		//void render(Renderer::Ptr renderer);

		void visitRenderable(std::function<void(Renderable)>)override {};

		const D3DXMATRIX& getViewMatrix();
		void lookat(const D3DXVECTOR3& eye, const D3DXVECTOR3& at, const D3DXVECTOR3& up = D3DXVECTOR3(0, 1, 0));
	private:
	};
public:
	Scene(Renderer::Ptr renderer);
	~Scene();

	Model::Ptr createModel(const std::string& name, const Parameters& params);
	Camera::Ptr createOrGetCamera(const std::string& name);

	void visitRenderables(std::function<void(Renderable)> callback);

	Node::Ptr getRoot() { return mRoot; }
	Node::Ptr createNode();
private:
	Renderer::Ptr mRenderer;
	std::map<std::string, Model::Ptr> mModels;
	Node::Ptr mRoot;

	std::map<std::string, Camera::Ptr> mCameras;
};