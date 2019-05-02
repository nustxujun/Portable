#include "Scene.h"

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

	Camera::Ptr c(new Camera());
	Node::Ptr node(new Node());
	node->mEntity = c;
	c->mNode = node;
	mCameras.emplace(name, c);
	return c;
}

void Scene::visitRenderables(std::function<void(Renderable)> callback)
{
	std::vector<Renderable> rends;

	std::function<void(Node::Ptr)> visitor;
	visitor = [&rends, &visitor](Node::Ptr node) {
		if (node->mEntity)
		{
			auto mat = node->mEntity->getNode()->getTransformation();
			node->mEntity->visitRenderable([&rends, mat](Renderable r) {
				r.tranformation = mat;
				rends.push_back(r);
			});
		}

		for (auto& n: node->mChildren)
		{
			visitor(n);
		}
	};

	visitor(mRoot);


	for (auto i : rends)
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



void Scene::Node::dirty()
{
	mDirty = true;
	for (auto c : mChildren)
	{
		c->dirty();
	}
}

const DirectX::XMFLOAT4X4 & Scene::Node::getTransformation()
{
	using namespace DirectX;
	if (mDirty)
	{
		mDirty = false;
		FXMVECTOR scaling = XMVectorSet(1, 1, 1, 0);
		FXMVECTOR o = XMVectorSet(0, 0, 0, 0);
		FXMVECTOR rot = XMLoadFloat4(&mOrientation);
		GXMVECTOR pos = XMLoadFloat3(&mPosition);
		XMMATRIX mat = DirectX::XMMatrixAffineTransformation(scaling,o,rot,pos);
		if (mParent)
		{
			XMMATRIX pmat = XMLoadFloat4x4(&mParent->getTransformation());
			mat = mat * pmat;
			//DirectX::XMMatrixMultiply(&mTranformation, &mTranformation, &mParent->getTransformation());
		}

		XMStoreFloat4x4(&mTranformation, mat);
	
	}
	return mTranformation;
}

Scene::Model::Model(const Parameters & params, Renderer::Ptr r)
{
	Mesh* mesh = new Mesh(params, r);
	mMesh = decltype(mMesh)(mesh);
}

Scene::Model::~Model()
{
}

void Scene::Model::visitRenderable(std::function<void(Renderable)> v)
{
	for (int i = 0; i < mMesh->getNumMesh(); ++i)
	{
		v(mMesh->getMesh(i));
	}
}

Scene::Camera::Camera():
	mFOVy(1.570796327f),
	mNear(0.01f), 
	mFar(10000.f),
	mProjectionType(PT_PERSPECTIVE), 
	mDirty(true)
{
}

Scene::Camera::~Camera()
{
}

const DirectX::XMFLOAT4X4& Scene::Camera::getViewMatrix()
{
	return mNode->getTransformation();
}

void Scene::Camera::lookat(const DirectX::XMFLOAT3 & eye, const DirectX::XMFLOAT3 & at, const DirectX::XMFLOAT3 & up)
{
	using namespace DirectX;
	
	XMMATRIX mat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3( &at), XMLoadFloat3(&up));


	XMVECTOR pos, scale, rot;
	DirectX::XMMatrixDecompose(&scale, &rot, &pos, mat);
	XMStoreFloat3(&mNode->mPosition, pos);
	XMStoreFloat4(&mNode->mOrientation, rot);
	mNode->dirty();
	//mNode->setPosition(pos);
	//mNode->setOrientation(rot);
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

const DirectX::XMFLOAT4X4 & Scene::Camera::getProjectionMatrix() 
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

//DirectX::XMFLOAT4 Scene::Camera::getDirection()
//{
//	DirectX::SimpleMath::Vector3 dir;
//	DirectX::SimpleMath::Quaternion q = mNode->getOrientation();
//
//
//	return DirectX::XMFLOAT4();
//}
//
