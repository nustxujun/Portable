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
	mOutputs.clear();
	for (int i = 0; i < outputs.size(); ++i)
	{
		if (!outputs[i].expired())
			mOutputs.push_back(outputs[i]->getView());
		else
			mOutputs.push_back(nullptr);

	}
}

void GPUComputer::setSamplers(const std::vector<Renderer::Sampler::Ptr>& samplers)
{
	mSamplers = samplers;
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

	//std::vector<ID3D11UnorderedAccessView*> uav;
	srvs.reserve(mOutputs.size());
	//for (auto&i : mOutputs)
		//if (i.lock())
			//uav.push_back(*i.lock());

	std::vector<ID3D11SamplerState*> smps;
	for (auto&i : mSamplers)
		smps.push_back(*i.lock());
	std::vector<ID3D11Buffer*> consts;
	consts.reserve(mConstants.size());
	for (auto&i : mConstants)
		if (i.lock())
			consts.push_back(*i.lock());

	auto context = mRenderer->getContext();
	context->CSSetShaderResources(0, srvs.size(), srvs.data());
	context->CSSetUnorderedAccessViews(0, mOutputs.size(), mOutputs.data(), NULL);
	context->CSSetConstantBuffers(0, consts.size(), consts.data());
	context->CSSetSamplers(0, smps.size(), smps.data());

	context->CSSetShader(*mCS.lock(), NULL, 0);
	context->Dispatch(x, y, z);
	
	std::vector<ID3D11ShaderResourceView*> srvclearer(srvs.size());
	context->CSSetShaderResources(0, srvs.size(), srvclearer.data());

	std::vector<ID3D11UnorderedAccessView*> clearer(mOutputs.size());
	context->CSSetUnorderedAccessViews(0, mOutputs.size(), clearer.data(),NULL);

}
