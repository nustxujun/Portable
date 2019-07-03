#include "Scene.h"
#include "MathUtilities.h"

Scene::Scene() 
{
	mRoot = Node::Ptr( new Node() );

}

Scene::~Scene()
{
}

Scene::Model::Ptr Scene::createModel(const std::string & name, const Parameters& params, Model::Loader loader)
{
	auto ret = mModels.find(name);
	if (ret != mModels.end())
		return ret->second;

	Model::Ptr e(new Model(params, loader));
	Node::Ptr node(new Node());
	node->mEntity = e;
	e->mNode = node;

	mModels.emplace(name, e);

	return e;
}

Scene::Model::Ptr Scene::getModel(const std::string& name)
{
	auto ret = mModels.find(name);
	if (ret != mModels.end())
		return ret->second;
	else
		return {};
}

Scene::Camera::Ptr Scene::createOrGetCamera(const std::string & name)
{
	auto ret = mCameras.find(name);
	if (ret != mCameras.end())
		return ret->second;

	Camera::Ptr c(new Camera(this));
	Node::Ptr node(new Node());
	node->mEntity = c;
	c->mNode = node;
	mCameras.emplace(name, c);
	return c;
}

Scene::Light::Ptr Scene::createOrGetLight(const std::string & name)
{
	auto ret = mLights.find(name);
	if (ret != mLights.end())
		return ret->second;

	Light::Ptr c(new Light());
	Node::Ptr node(new Node());
	node->mEntity = c;
	c->mNode = node;
	mLights.emplace(name, c);
	return c;
}

void Scene::visitRenderables(std::function<void(const Renderable&)> callback, std::function<bool(Entity::Ptr)> cond)
{
	std::vector<Renderable> rends;

	std::function<void(Node::Ptr)> visitor;
	visitor = [&rends, &visitor,cond](Node::Ptr node) {
		if (node->mEntity && (!cond || cond(node->mEntity)))
		{
			auto matrix = node->mEntity->getNode()->getTransformation();
			node->mEntity->visitRenderable([=, &rends](const Renderable& r) {
				Renderable rend = r;
				rend.tranformation *= matrix;
				rends.emplace_back(std::move(rend));
			});
		}

		for (auto& n: node->mChildren)
		{
			visitor(n);
		}
	};

	visitor(mRoot);


	for (auto& i : rends)
	{
		callback(i);
	}
}

void Scene::visitLights(std::function<void(Light::Ptr)> callback)
{
	for (auto& i : mLights)
	{
		callback(i.second);
	}
}

Scene::Node::Ptr Scene::createNode(const std::string& name)
{
	if (name.size() == 0)
		return std::make_shared<Node>();

	auto ret = mNodes.find(name);
	if (ret != mNodes.end())
		return ret->second;

	auto ptr = std::make_shared<Node>();
	mNodes[name] = ptr;
	return ptr;
}

Scene::Probe::Ptr Scene::createProbe(const std::string & name)
{
	auto ret = mProbes.find(name);
	if (ret != mProbes.end())
		return ret->second;

	Node::Ptr node(new Node());
	auto probe = Probe::Ptr(new Probe());
	node->mEntity = probe;
	probe->mNode = node;
	mProbes[name] = probe;
	return probe;
}


Scene::Entity::Entity()
{
}

Scene::Entity::~Entity()
{
}

Vector3 Scene::Entity::getDirection()
{
	Vector3 dir(0, 0, 1);
	auto node = getNode();
	const auto& rot = node->getRealOrientation();
	Vector3::Transform(dir, rot, dir);
	dir.Normalize();
	return dir;
}

void Scene::Entity::setDirection(const Vector3 & dir)
{
	using namespace DirectX;

	Vector3 d = dir;
	d.Normalize();
	Vector3 up(0, 1, 0);

	if (fabs(up.Dot(dir)) == 1.0f)
		up = { 0 ,0, 1 };

	Vector3 x = up.Cross(d);
	x.Normalize();
	Vector3 y = d.Cross(x);
	y.Normalize();

	Matrix  mat = MathUtilities::makeMatrixFromAxis(x, y, d);

	mNode->setOrientation(Quaternion::CreateFromRotationMatrix(mat));

	Vector3 t(0, 0, 1);
	t = Vector3::Transform(t, mNode->getOrientation());
}


void Scene::Camera::visitVisibleObject(std::function<void(Entity::Ptr)> visit)
{
	for (auto& e : mScene->mModels)
		visit(e.second);
}


std::pair<Vector3, Vector3> Scene::Node::getWorldAABB() const
{
	std::pair<Vector3, Vector3> aabb = { {FLT_MAX,FLT_MAX,FLT_MAX}, {FLT_MIN,FLT_MIN,FLT_MIN} };

	for (auto &i : mChildren)
	{
		auto bb = i->getWorldAABB();
		aabb.first = Vector3::Min(aabb.first, bb.first);
		aabb.second = Vector3::Max(aabb.second, bb.second);
	}

	if (mEntity)
	{
		auto bb = mEntity->getWorldAABB();
		aabb.first = Vector3::Min(aabb.first, bb.first);
		aabb.second = Vector3::Max(aabb.second, bb.second);
	}

	return aabb;
}

void Scene::Node::dirty(size_t s)
{
	mDirty |= s;
	mDirty |= DT_TRANS;
	for (auto c : mChildren)
	{
		c->dirty(s);
	}
}

bool Scene::Node::isDirty(size_t s)
{
	return (s & mDirty) != 0;
}

const Matrix & Scene::Node::getTransformation()
{
	using namespace DirectX;
	if (isDirty())
	{
		FXMVECTOR scaling = XMVectorSet(1, 1, 1, 0);
		FXMVECTOR o = XMVectorSet(0, 0, 0, 0);
		FXMVECTOR rot = getOrientation();
		GXMVECTOR pos = getPosition();
		XMMATRIX mat = DirectX::XMMatrixAffineTransformation(scaling,o,rot, pos);
		if (mParent)
		{
			XMMATRIX pmat = mParent->getTransformation();
			mat = mat * pmat;
		}

		XMStoreFloat4x4(&mTranformation, mat);

		mDirty = mDirty & ~(DT_TRANS);
	}
	return mTranformation;
}

const Vector3 & Scene::Node::getRealPosition()
{
	if (isDirty( DT_POSITION ))
	{
		mRealPosition = mPosition;
		if (mParent)
		{
			mRealPosition = Vector3::Transform(mPosition, mParent->getRealOrientation());
			mRealPosition += mParent->getRealPosition();
		}
		mDirty &= ~DT_POSITION;
	}

	return mRealPosition;
}

const Quaternion & Scene::Node::getRealOrientation()
{
	if (isDirty (DT_ORIENTATION))
	{
		mRealOrientation = mOrientation;
		if (mParent)
		{
			mRealOrientation *= mParent->getOrientation();
		}
		mDirty &= ~DT_ORIENTATION;
	}

	return mRealOrientation;
}


Scene::Model::Model(const Parameters & params, Loader loader)
{
	mMesh = loader(params);
}

Scene::Model::~Model()
{
}

void Scene::Model::visitRenderable(std::function<void(const Renderable&)> v)
{
	for (int i = 0; i < mMesh->getNumMesh(); ++i)
	{
		v(mMesh->getMesh(i));
	}
}

void Scene::Model::setMaterial(Material::Ptr mat)
{
	mMesh->setMaterial(mat);
}

std::pair<Vector3, Vector3> Scene::Model::getWorldAABB() const
{
	auto aabb = mMesh->getAABB();

	auto node = getNode();

	const auto& tran = node->getTransformation();
	aabb.min = Vector3::Transform(aabb.min, tran);
	aabb.max = Vector3::Transform(aabb.max, tran);
	Vector3 min = Vector3::Min(aabb.min, aabb.max);
	Vector3 max = Vector3::Max(aabb.min, aabb.max);

	return std::pair<Vector3, Vector3>(min, max);
}

Scene::Camera::Camera(Scene* s):
	mFOVy(1.570796327f),
	mNear(0.01f), 
	mFar(10000.f),
	mProjectionType(PT_PERSPECTIVE),
	mScene(s)
{
}

Scene::Camera::~Camera()
{
}

Matrix Scene::Camera::getViewMatrix()
{
	return MathUtilities::makeViewMatrix(mNode->getRealPosition(), mNode->getRealOrientation());
}

void Scene::Camera::setViewMatrix(const Matrix& mat)
{
	auto conv = mat;
	mNode->setPosition(-mat.Translation());
	conv.Translation(-mat.Translation());
	Quaternion q = Quaternion::CreateFromRotationMatrix(conv.Transpose());
	mNode->setOrientation(q);
}

void Scene::Camera::lookat(const Vector3 & eye, const Vector3 & at)
{
	setDirection(at - eye);
	mNode->setPosition(eye);
}

void Scene::Camera::setViewport(float left, float top, float width, float height)
{
	mDirty = true;
	mViewport.Width = width;
	mViewport.Height = height;
	mViewport.TopLeftX = left;
	mViewport.TopLeftY = top;
	mViewport.MinDepth = 0.f;
	mViewport.MaxDepth = 1.0f;
}

void Scene::Camera::setFOVy(float fovy)
{
	if (mProjectionType == PT_PERSPECTIVE)
		mDirty = true;
	mFOVy = fovy;
}

void Scene::Camera::setNearFar(float n, float f)
{
	mDirty = true;
	mNear = n;
	mFar = f;
}

const Matrix & Scene::Camera::getProjectionMatrix() 
{
	if (mDirty)
	{
		using namespace DirectX;
		XMMATRIX mat;
		if (mProjectionType == PT_ORTHOGRAPHIC)
		{
			mat = XMMatrixOrthographicLH(mViewport.Width, mViewport.Height, mNear, mFar);
		}
		else
		{
			mat = XMMatrixPerspectiveFovLH(mFOVy, mViewport.Width / mViewport.Height, mNear, mFar);
		}
		XMStoreFloat4x4(&mProjection, mat);
	}
	return mProjection;
}

std::array<Scene::Camera::Corner, 8> Scene::Camera::getWorldCorners()
{
	auto ret = MathUtilities::calFrustumCorners(mViewport.Width, mViewport.Height, mNear, mFar, mFOVy);
	auto eyeToWorld = getViewMatrix().Invert();
	for (auto&i : ret)
		i = Vector3::Transform(i, eyeToWorld);


	std::array<Scene::Camera::Corner, 8> arr ;
	return ret;
}


Scene::Light::Light()
{
}

Scene::Light::~Light()
{
}

Scene::Probe::Probe()
{
//#ifdef _DEBUG
//	
//#endif
}

void Scene::Probe::setDebugObject(Mesh::Ptr m) 
{ 
	mDebugObject = m; 
}

bool Scene::Probe::intersect(const Vector3 & point)
{
	auto aabb = getWorldAABB();

	return
		!(aabb.second.x < point.x || aabb.first.x > point.x ||
			aabb.second.y < point.y || aabb.first.y > point.y ||
			aabb.second.z < point.z || aabb.first.z > point.z);
}

void Scene::Probe::visitRenderable(std::function<void(const Renderable&)> v)
{
	if (!mDebugObject)
		return;
	for (int i = 0; i < mDebugObject->getNumMesh(); ++i)
	{
		v(mDebugObject->getMesh(i));
	}

}

std::pair<Vector3, Vector3> Scene::Probe::getWorldAABB() const
{
	auto pos = getNode()->getRealPosition() + mInfluence.second;

	return { pos - mInfluence.first * 0.5f , pos + mInfluence.first * 0.5f };
}
