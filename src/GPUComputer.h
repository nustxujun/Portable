#pragma once

#include "renderer.h"

class GPUComputer
{
public:
	using Ptr = std::shared_ptr<GPUComputer>;
public:
	GPUComputer(Renderer::Ptr r);
	~GPUComputer();

	void setConstants(const std::vector<Renderer::Buffer::Ptr>& consts);
	void setInputs(const std::vector<Renderer::ShaderResource::Ptr>& inputs);
	void setOuputs(const std::vector<Renderer::UnorderedAccess::Ptr>& outputs);
	void setShader(Renderer::ComputeShader::Weak cs);

	void compute(UINT x, UINT y, UINT z);
private:
	std::vector<Renderer::ShaderResource::Ptr> mInputs;
	std::vector<Renderer::UnorderedAccess::Ptr> mOutputs;
	std::vector<Renderer::Buffer::Ptr> mConstants;
	Renderer::ComputeShader::Weak mCS;
	Renderer::Ptr mRenderer;
};