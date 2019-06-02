#include "ClusteredLightCulling.h"

void ClusteredLightCulling::init(const Vector3& slices, const Vector3& clustersize)
{
	set("clustered", { {"value",true} });
	mSlices = slices;
	mCluster = clustersize;
	mName = "clustered light culling";
	mComputer = GPUComputer::Ptr(new GPUComputer(getRenderer()));
	mCS = getRenderer()->createComputeShader("hlsl/clusteredlightculling.hlsl","main");
	mConstants = getRenderer()->createBuffer(sizeof(Constants), D3D11_BIND_CONSTANT_BUFFER, 0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mCurIndex = getRenderer()->createRWBuffer(4, 4, DXGI_FORMAT_R32_UINT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

}

void ClusteredLightCulling::genFrustums()
{
	//struct Frustum
	//{
	//	Vector4 faces[6];
	//};

	//const Vector3 LEFT_NORMAL = Vector3(-1, 0, 0);
	//const Vector3 FRONT_NORMAL = Vector3(0, 0, -1);
	//const Vector3 RIGHT_NORMAL = Vector3(1, 0, 0);
	//const Vector3 BACK_NORMAL = Vector3(0, 0, 1);
	//const Vector3 TOP_NORMAL = Vector3(0, 1, 0);
	//const Vector3 BOTTOM_NORMAL = Vector3(0, -1, 0);

	//enum
	//{
	//	LEFT,
	//	RIGHT,
	//	FRONT,
	//	BACK,
	//	TOP,
	//	BOTTOM
	//};

	//int num = mSlices.x * mSlices.y * mSlices.z;
	//std::vector<Frustum> frustums(num);
	//for (int z = 0; z <  mSlices.z; ++z )
	//	for (int y = 0; y < mSlices.y; ++y)
	//		for (int x = 0; x < mSlices.x; ++x)
	//		{
	//			int index = x + y * mSlices.x + z * mSlices.x * mSlices.y;
	//			auto& f = frustums[index].faces;
	//			f[LEFT] = Vector4(LEFT_NORMAL.x, LEFT_NORMAL.y, LEFT_NORMAL.z, position.Length());
	//			f[RIGHT] = Vector4(RIGHT_NORMAL.x, RIGHT_NORMAL.y, RIGHT_NORMAL.z, (position + Vector3(dimensions.x, 0, 0)).Length());
	//			f[FRONT] = Vector4(FRONT_NORMAL.x, FRONT_NORMAL.y, FRONT_NORMAL.z, (position + Vector3(0, 0, dimensions.z)).Length());
	//			f[BACK] = Vector4(BACK_NORMAL.x, BACK_NORMAL.y, BACK_NORMAL.z, position.Length());
	//			f[TOP] = Vector4(TOP_NORMAL.x, TOP_NORMAL.y, TOP_NORMAL.z, (position + Vector3(0, dimensions.y, 0)).Length());
	//			f[BOTTOM] = Vector4(BOTTOM_NORMAL.x, BOTTOM_NORMAL.y, BOTTOM_NORMAL.z, position.Length());
	//			
	//		}


}


void ClusteredLightCulling::render(Renderer::Texture2D::Ptr rt)
{
	std::array<UINT, 4> clear = {0,0,0,0};
	mCurIndex.lock()->clear( clear);

	mComputer->setInputs({ getShaderResource("lights") });
	mComputer->setOuputs({ mCurIndex ,getUnorderedAccess("lightindices"), getUnorderedAccess("clusteredlights") });
	
	Constants constants;
	auto cam = getScene()->createOrGetCamera("main");
	constants.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	constants.farZ = cam->getFar();
	constants.nearZ = cam->getNear();
	constants.numLights = getValue<int>("numLights");
	constants.texelwidth = 1.0f / (float)mSlices.x;
	constants.texelheight= 1.0f / (float)mSlices.y;
	constants.invclusterwidth = 1.0f / mCluster.x;
	constants.invclusterheight = 1.0f / mCluster.y;

	mConstants.lock()->blit(constants);

	mComputer->setConstants({ mConstants });
	mComputer->setShader(mCS);
	mComputer->compute(mSlices.x, mSlices.y, mSlices.z);
}

