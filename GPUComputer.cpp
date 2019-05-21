#include "GPUComputer.h"

GPUComputer::GPUComputer(Renderer::Ptr  r):mRenderer(r)
{
}

GPUComputer::~GPUComputer()
{
}

void GPUComputer::setConstants(const std::vector<Renderer::Buffer::Ptr>& consts)
{
	mConstants = consts;
}

void GPUComputer::setInputs(const std::vector<Renderer::ShaderResource::Ptr>& inputs)
{
	mInputs = inputs;
}

void GPUComputer::setOuputs(const std::vector<Renderer::UnorderedAccess::Ptr>& outputs)
{
	mOutputs = outputs;
}

void GPUComputer::setShader(Renderer::ComputeShader::Weak cs)
{
	mCS = cs;
}

void GPUComputer::compute(UINT x, UINT y, UINT z)
{
	std::vector<ID3D11ShaderResourceView*> srvs;
	srvs.reserve(mInputs.size());
	for (auto&i : mInputs)
		if (i.lock())
			srvs.push_back(*i.lock());

	std::vector<ID3D11UnorderedAccessView*> uav;
	srvs.reserve(mOutputs.size());
	for (auto&i : mOutputs)
		if (i.lock())
			uav.push_back(*i.lock());

	std::vector<ID3D11Buffer*> consts;
	consts.reserve(mConstants.size());
	for (auto&i : mConstants)
		if (i.lock())
			consts.push_back(*i.lock());

	auto context = mRenderer->getContext();
	context->CSSetShaderResources(0, srvs.size(), srvs.data());
	context->CSSetUnorderedAccessViews(0, uav.size(), uav.data(), NULL);
	context->CSSetConstantBuffers(0, consts.size(), consts.data());

	mRenderer->getContext()->Dispatch(x, y, z);
	mRenderer->removeShaderResourceViews();

	std::vector<ID3D11UnorderedAccessView*> clearer(uav.size());
	context->CSSetUnorderedAccessViews(0, uav.size(), clearer.data(),NULL);
}
