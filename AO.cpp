#include "AO.h"
#include <algorithm>
#include <random>

AO::AO(Renderer::Ptr r, Scene::Ptr s, Pipeline* p): Pipeline::Stage(r,s,p)
{
	mNormal = getSharedRT("normal");
	mDepth = getSharedRT("depth");
	auto blob = r->compileFile("hlsl/ssao.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());


	mKernel = r->createBuffer(sizeof(Kernel), D3D11_BIND_CONSTANT_BUFFER);
	mMatrix = r->createBuffer(sizeof(Matrix), D3D11_BIND_CONSTANT_BUFFER);

	std::uniform_real_distribution<float> rand1(-1.0, 1.0); 
	std::uniform_real_distribution<float> rand2(0 , 1.0);
	std::default_random_engine gen;

	float noiseSize = 4.0f;

	Kernel kernel;
	for (int i = 0; i < 64; ++i)
	{
		DirectX::SimpleMath::Vector3 v = { rand1(gen) ,rand1(gen) ,rand2(gen) };
		v.Normalize();
		v *= pow( rand2(gen),2);
		kernel.kernel[i] = { v.x, v.y,v.z };
	}
	kernel.scale = { r->getWidth() / noiseSize, r->getHeight() / noiseSize };
	kernel.kernelSize = 64;
	kernel.radius = 1;
	mKernel.lock()->blit(&kernel, sizeof(kernel));

	D3D11_TEXTURE2D_DESC noiseTexDesc;
	noiseTexDesc.Width = noiseSize;
	noiseTexDesc.Height = noiseSize;
	noiseTexDesc.MipLevels = 1;
	noiseTexDesc.ArraySize = 1;
	noiseTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	noiseTexDesc.SampleDesc.Count = 1;
	noiseTexDesc.SampleDesc.Quality = 0;
	noiseTexDesc.Usage = D3D11_USAGE_DEFAULT;
	noiseTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	noiseTexDesc.CPUAccessFlags = 0;
	noiseTexDesc.MiscFlags = 0;

	float noise[16] = { 0 };
	for (int i = 0; i < 4; ++i)
	{
		DirectX::SimpleMath::Vector3 d = { rand1(gen), rand1(gen), 0.0f };
		memcpy(noise + i * 4, &d, sizeof(d));
	}

	mNoise = r->createTexture("ssao_noise", noiseTexDesc, noise, sizeof(noise));

	mLinearWrap = r->createSampler("linear_wrap", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
}

void AO::render(Renderer::RenderTarget::Ptr rt)
{
	using namespace DirectX;
	auto cam = getScene()->createOrGetCamera("main");

	Matrix matrix;

	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();
	auto mat = view * proj;

	XMStoreFloat4x4A(&matrix.invertProjection,mat.Invert());
	XMStoreFloat4x4A(&matrix.projection, mat);
	mMatrix.lock()->blit(&matrix, sizeof(matrix));


	mQuad->setConstants({ mKernel, mMatrix });
	mQuad->setPixelShader(mPS);
	mQuad->setSamplers({ mLinearWrap });
	mQuad->setTextures({ mNormal.lock()->getShaderResourceView(), mDepth.lock()->getShaderResourceView(), *mNoise.lock() });
	D3D11_BLEND_DESC desc = { 0 };
	desc.RenderTarget[0] = {
		FALSE,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE,
		D3D11_BLEND_ZERO,
		D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};

	mQuad->setBlend(desc);
	mQuad->setRenderTarget(rt);
	mQuad->draw();
}


