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
			DT_TRANS			= 5,

			DT_ALL				= DT_POSITION | DT_ORIENTATION | DT_PARENT,
		};
	public:
		using Ptr = std::shared_ptr<Node>;
	public:
		Node() :mPosition(0, 0, 0), mOrientation(0, 0, 0, 1) {};

		const Matrix& getTransformation();

		void setPosition(float x, float y, float z) { dirty(DT_POSITION); mPosition = {x,y,z}; }
		void setPosition(const Vector3& p) { dirty(DT_POSITION); mPosition = p; }

		const Vector3& getPosition() { return mPosition; }
		const Vector3& getRealPosition();

		template<class ... Args>
		void setOrientation(const Args& ... args) { dirty(DT_ORIENTATION); mOrientation = { args... }; }
		const Quaternion& getOrientation() { return mOrientation; }
		const Quaternion& getRealOrientation();
		void rotate(const Quaternion& rot) { setOrientation( mOrientation * rot); }
		void rotate(float pitch, float yaw, float roll) {
			Quaternion rot = Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
			rotate(rot);
		}

		void addChild(Ptr p) { mChildren.insert(p); p->mParent = this; p->dirty(DT_PARENT); }
		void removeChild(Ptr p) { mChildren.erase(p); p->mParent = nullptr; p->dirty(DT_PARENT); }

		std::pair<Vector3, Vector3> getWorldAABB()const;
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
		virtual void setMaterial(Material::Ptr mat) { abort(); };
		virtual std::pair<Vector3, Vector3> getWorldAABB()const = 0;
		void attach(Node::Ptr n) { n->addChild(mNode); };
		void detach() { if (!mNode->mParent) return; mNode->mParent->removeChild(mNode); }

		Node::Ptr getNode()const { return mNode; }

		Vector3 getDirection();
		void setDirection(const Vector3& dir);

		bool isCastShadow()const { return mCastShadow; }
		void setCastShadow(bool s) { mCastShadow = s; }

	private:
		Node::Ptr mNode;
		bool mCastShadow = true;
	};

	class Model : public Entity
	{
	public:
		using Ptr = std::shared_ptr<Model>;
		using Loader = std::function<Mesh::Ptr(const Parameters&)>;
	public:
		Model(const Parameters& params, Loader loader);
		~Model();
		Mesh::Ptr getMesh() { return mMesh; };
		virtual void visitRenderable(std::function<void(const Renderable&)>) override;
		virtual void setMaterial(Material::Ptr mat) override;
		virtual std::pair<Vector3, Vector3> getWorldAABB()const override;


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

		using Corner = DirectX::SimpleMath::Vector3;
	public:
		Camera(Scene* s);
		~Camera();

		//void render(Renderer::Ptr renderer);

		void visitRenderable(std::function<void(const Renderable&)>)override {};
		virtual std::pair<Vector3, Vector3> getWorldAABB()const override { return std::pair<Vector3, Vector3>(); };

		Matrix getViewMatrix();
		void setViewMatrix(const Matrix& mat);
		void lookat(const Vector3& eye, const Vector3& at);
		void setViewport(float left, float top, float width, float height);
		void setFOVy(float fovy);
		float getFOVy()const { return mFOVy; }
		void setNearFar(float n, float f);
		float getNear()const { return mNear; }
		float getFar()const { return mFar; }

		void setProjectType(ProjectionType type) { mProjectionType = type; };
		const Matrix & getProjectionMatrix();
		const D3D11_VIEWPORT& getViewport()const { return mViewport; }
		std::array<Corner, 8> getWorldCorners();

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

	class Light : public Entity
	{
	public:
		using Ptr = std::shared_ptr<Light>;
		enum LightType
		{
			LT_POINT,
			LT_DIR,
			LT_SPOT,
		};
	public:
		Light();
		~Light();

		void visitRenderable(std::function<void(const Renderable&)>)override {};
		std::pair<Vector3, Vector3> getWorldAABB()const override { return std::pair<Vector3, Vector3>(); };
		void setColor(const Vector3& c) { mColor = c; }
		const Vector3& getColor()const { return mColor; }
		void setRange(float r) { mRange = r; }
		float getRange()const { return mRange; }
		void setType(LightType t) { mType = t; }
		LightType getType()const { return mType; }
		void setSpotAngle(float ang) { mAngle = ang; }
		float getSpotAngle()const { return mAngle; }
		void setCastingShadow(bool v) { mIsCastingShadow = v; }
		bool isCastingShadow()const { return mIsCastingShadow; }
	private:
		Vector3 mColor = {1,1,1};
		float mRange = 1000.0f;
		LightType mType = LT_DIR;
		float mAngle = 0.7854f;
		bool mIsCastingShadow = false;
	};
public:
	Scene();
	~Scene();

	Model::Ptr createModel(const std::string& name, const Parameters& params, Model::Loader loader);
	Model::Ptr getModel(const std::string& name);
	Camera::Ptr createOrGetCamera(const std::string& name);
	Light::Ptr createOrGetLight(const std::string& name);
	void visitRenderables(std::function<void(const Renderable&)> callback, std::function<bool(Entity::Ptr)> cond = {});
	void visitLights(std::function<void(Light::Ptr)> callback);
	size_t getNumLights()const { return mLights.size(); }
	Node::Ptr getRoot() { return mRoot; }
	Node::Ptr createNode(const std::string& name = {});
private:
	std::unordered_map<std::string, Model::Ptr> mModels;
	Node::Ptr mRoot;

	std::unordered_map<std::string, Camera::Ptr> mCameras;
	std::unordered_map<std::string, Light::Ptr> mLights;
	std::unordered_map<std::string, Node::Ptr> mNodes;

};