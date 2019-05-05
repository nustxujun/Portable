#include "Scene.h"
#include "MathUtilities.h"

Scene::Scene(Renderer::Ptr r) : mRenderer(r)
{
	mRoot = Node::Ptr( new Node() );

}

Scene::~Scene()
{
}

Scene::Model::Ptr Scene::createModel(const std::string & name, const Parameters& params)
{
	auto ret = mModels.find(name);
	if (ret != mModels.end())
		return ret->second;

	Model::Ptr e(new Model(params, mRenderer));
	Node::Ptr node(new Node());
	node->mEntity = e;
	e->mNode = node;

	mModels.emplace(name, e);

	return e;
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

void Scene::visitRenderables(std::function<void(const Renderable&)> callback)
{
	std::vector<Renderable> rends;

	std::function<void(Node::Ptr)> visitor;
	visitor = [&rends, &visitor](Node::Ptr node) {
		if (node->mEntity)
		{
			auto mat = node->mEntity->getNode()->getTransformation();
			node->mEntity->visitRenderable([&rends, mat](const Renderable& r) {
				Renderable rend = r;
				rend.tranformation = mat;
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

Scene::Node::Ptr Scene::createNode()
{
	Node::Ptr ptr(new Node());
	return ptr;
}

Scene::Entity::Entity()
{
}

Scene::Entity::~Entity()
{
}



void Scene::Node::dirty(size_t s)
{
	mDirty |= s;
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
		FXMVECTOR rot = getRealOrientation();
		GXMVECTOR pos = getRealPosition();
		XMMATRIX mat = DirectX::XMMatrixAffineTransformation(scaling,o,rot, pos);
		if (mParent)
		{
			XMMATRIX pmat = mParent->getTransformation();
			mat = mat * pmat;
		}

		XMStoreFloat4x4(&mTranformation, mat);
		mDirty = 0;
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

Scene::Model::Model(const Parameters & params, Renderer::Ptr r)
{
	Mesh* mesh = new Mesh(params, r);
	mMesh = decltype(mMesh)(mesh);
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

Vector3 Scene::Camera::getDirection()
{
	Vector3 dir(0,0,1);
	auto node = getNode();
	const auto& rot = node->getOrientation();
	Vector3::Transform(dir, rot, dir);
	dir.Normalize();
	return dir;
}

void Scene::Camera::setDirection(const Vector3 & dir)
{
	using namespace DirectX;

	Vector3 d = dir;
	d.Normalize();
	Vector3 up(0, 1, 0);

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

