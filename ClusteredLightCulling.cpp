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


void ClusteredLightCulling::render(Renderer::Texture2D::Ptr rt)
{
	std::array<UINT, 4> clear = {0,0,0,0};
	mCurIndex.lock()->clear( clear);

	mComputer->setInputs({ getShaderResource("pointlights") });
	mComputer->setOuputs({ mCurIndex ,getUnorderedAccess("lightindices"), getUnorderedAccess("lighttable") });
	
	Constants constants;
	auto cam = getScene()->createOrGetCamera("main");
	constants.invertProj = cam->getProjectionMatrix().Invert().Transpose();
	constants.farZ = cam->getFar();
	constants.nearZ = cam->getNear();
	constants.numLights = getValue<int>("numLights");
	constants.slicesSize = mSlices;

	mConstants.lock()->blit(constants);

	mComputer->setConstants({ mConstants });
	mComputer->setShader(mCS);
	mComputer->compute(mSlices.x, mSlices.y, mSlices.z);
}

