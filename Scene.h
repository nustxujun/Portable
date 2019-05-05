#pragma once
#include "Common.h"
#include "Mesh.h"
#include "SimpleMath.h"

using namespace DirectX::SimpleMath;
class Scene
{

public:
	using Ptr = std::shared_ptr<Scene>;
	class Entity;
	class Node
	{
		friend class Scene;

		enum DIRTY_TYPE{
			DT_POSITION			= 1,
			DT_ORIENTATION		= 2,
			DT_PARENT			= 4,


			DT_ALL				= DT_POSITION | DT_ORIENTATION | DT_PARENT,
		};
	public:
		using Ptr = std::shared_ptr<Node>;
	public:
		Node() :mPosition(0, 0, 0), mOrientation(0, 0, 0, 1) {};

		const Matrix& getTransformation();

		template<class ... Args>
		void setPosition(const Args& ... args) { dirty(DT_POSITION); mPosition = { args... }; }
		const Vector3& getPosition() { return mPosition; }
		const Vector3& getRealPosition();

		template<class ... Args>
		void setOrientation(const Args& ... args) { dirty(DT_ORIENTATION); mOrientation = { args... }; }
		const Quaternion& getOrientation() { return mOrientation; }
		const Quaternion& getRealOrientation();
		

		void addChild(Ptr p) { mChildren.insert(p); p->mParent = this; p->dirty(DT_PARENT); }
		void removeChild(Ptr p) { mChildren.erase(p); p->mParent = nullptr; p->dirty(DT_PARENT); }
	private:
		void dirty(size_t s);
		bool isDirty(size_t s = DT_ALL);
	private:
		Node* mParent;
		std::set<Ptr> mChildren;
		std::shared_ptr<Entity> mEntity;
		Vector3 mPosition;
		Vector3 mRealPosition;
		Quaternion mOrientation;
		Quaternion mRealOrientation;
		Matrix mTranformation;
		size_t mDirty = DT_ALL;
	};

	class Entity
	{
		friend class Scene;
	public:
		using Ptr = std::shared_ptr<Entity>;
	public:
		Entity();
		~Entity();

		virtual void visitRenderable(std::function<void(const Renderable&)>) = 0;
		virtual std::pair<Vector3, Vector3> getWorldAABB()const = 0;
		void attach(Node::Ptr n) { n->addChild(mNode); };
		void detach() { if (!mNode->mParent) return; mNode->mParent->removeChild(mNode); }

		Node::Ptr getNode()const { return mNode; }
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
		virtual void visitRenderable(std::function<void(const Renderable&)>) override;
		virtual std::pair<Vector3, Vector3> getWorldAABB()const;

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
		Camera(Scene* s);
		~Camera();

		//void render(Renderer::Ptr renderer);

		void visitRenderable(std::function<void(const Renderable&)>)override {};
		virtual std::pair<Vector3, Vector3> getWorldAABB()const override { return std::pair<Vector3, Vector3>(); };

		Matrix getViewMatrix();
		void lookat(const Vector3& eye, const Vector3& at);
		void setViewport(float left, float top, float width, float height);
		void setFOVy(float fovy);
		void setNearFar(float n, float f);
		void setProjectType(ProjectionType type) { mProjectionType = type; };
		const Matrix & getProjectionMatrix();
		const D3D11_VIEWPORT& getViewport()const { return mViewport; }

		Vector3 getDirection();
		void setDirection(const Vector3& dir);

		void visitVisibleObject(std::function<void(Entity::Ptr)> visit);
	private:
		D3D11_VIEWPORT mViewport;
		float mFOVy;
		float mNear;
		float mFar;
		ProjectionType mProjectionType;
		Matrix mProjection;
		bool mDirty;
		Scene* mScene;
	};
public:
	Scene(Renderer::Ptr renderer);
	~Scene();

	Model::Ptr createModel(const std::string& name, const Parameters& params);
	Camera::Ptr createOrGetCamera(const std::string& name);

	void visitRenderables(std::function<void(const Renderable&)> callback);

	Node::Ptr getRoot() { return mRoot; }
	Node::Ptr createNode();
private:
	Renderer::Ptr mRenderer;
	std::unordered_map<std::string, Model::Ptr> mModels;
	Node::Ptr mRoot;

	std::unordered_map<std::string, Camera::Ptr> mCameras;
};