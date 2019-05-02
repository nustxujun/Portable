#pragma once
#include "Common.h"
#include "Mesh.h"
#include <DirectXMath.h>


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

		const DirectX::XMFLOAT4X4& getTransformation();

		template<class ... Args>
		void setPosition(const Args& ... args) { dirty(); mPosition = { args... }; }
		const DirectX::XMFLOAT3& getPosition() { return mPosition; }

		template<class ... Args>
		void setOrientation(const Args& ... args) { dirty(); mOrientation = { args... }; }
		const DirectX::XMFLOAT4& getOrientation() { return mOrientation; }


		void addChild(Ptr p) { mChildren.insert(p); p->mParent = this; p->dirty(); }
		void removeChild(Ptr p) { mChildren.erase(p); p->mParent = nullptr; p->dirty(); }
	private:
		void dirty();

	private:
		Node* mParent;
		std::set<Ptr> mChildren;
		std::shared_ptr<Entity> mEntity;
		DirectX::XMFLOAT3 mPosition;
		DirectX::XMFLOAT4 mOrientation;
		DirectX::XMFLOAT4X4 mTranformation;
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

		enum ProjectionType
		{
			PT_ORTHOGRAPHIC,
			PT_PERSPECTIVE,
		};
	public:
		Camera();
		~Camera();

		//void render(Renderer::Ptr renderer);

		void visitRenderable(std::function<void(Renderable)>)override {};

		const DirectX::XMFLOAT4X4& getViewMatrix();
		void lookat(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& at, const DirectX::XMFLOAT3& up = DirectX::XMFLOAT3(0, 1, 0));
		void setViewport(float left, float top, float width, float height);
		void setFOVy(float fovy);
		void setNearFar(float n, float f);
		void setProjectType(ProjectionType type) { mProjectionType = type; };
		const DirectX::XMFLOAT4X4 & getProjectionMatrix();
		const D3D11_VIEWPORT& getViewport()const { return mViewport; }

		DirectX::XMFLOAT4 getDirection();
	private:
		D3D11_VIEWPORT mViewport;
		float mFOVy;
		float mNear;
		float mFar;
		ProjectionType mProjectionType;
		DirectX::XMFLOAT4X4 mProjection;
		bool mDirty;
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