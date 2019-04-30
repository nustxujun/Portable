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

const D3DXMATRIX & Scene::Node::getTransformation() 
{
	if (mDirty)
	{
		mDirty = false;
		D3DXVECTOR3 scaling(1, 1, 1);
		D3DXMatrixTransformation(&mTranformation, NULL, NULL, &scaling, NULL, &mOrientation, &mPosition);
		if (mParent)
		{
			D3DXMatrixMultiply(&mTranformation, &mTranformation, &mParent->getTransformation());
		}
	
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

Scene::Camera::Camera()
{
}

Scene::Camera::~Camera()
{
}

const D3DXMATRIX& Scene::Camera::getViewMatrix()
{
	return mNode->getTransformation();
}

void Scene::Camera::lookat(const D3DXVECTOR3 & eye, const D3DXVECTOR3 & at, const D3DXVECTOR3 & up)
{
	D3DXMATRIX mat;
	D3DXMatrixLookAtLH(&mat, &eye, &at, &up);

	D3DXVECTOR3 pos, scaling;
	D3DXQUATERNION rot;
	D3DXMatrixDecompose(&scaling, &rot, &pos, &mat);
	mNode->setPosition(pos);
	mNode->setOrientation(rot);
}

