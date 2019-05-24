#include "AO.h"
#include <algorithm>
#include <random>
#include "MathUtilities.h"

AO::AO(Renderer::Ptr r, Scene::Ptr s, Setting::Ptr st, Pipeline* p, Renderer::ShaderResource::Ptr normal, Renderer::ShaderResource::Ptr depth, float radius):
	Pipeline::Stage(r,s,st,p), mNormal(normal), mDepth(depth)
{
	auto blob = r->compileFile("hlsl/ssao.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader((*blob)->GetBufferPointer(), (*blob)->GetBufferSize());


	mKernel = r->createBuffer(sizeof(Kernel), D3D11_BIND_CONSTANT_BUFFER);
	mMatrix = r->createBuffer(sizeof(ConstantMatrix), D3D11_BIND_CONSTANT_BUFFER);

	std::uniform_real_distribution<float> rand1(-1.0, 1.0); 
	std::uniform_real_distribution<float> rand2(0.0 , 1.0);
	std::uniform_real_distribution<float> rand3(0.0, 1.0);

	std::default_random_engine gen;

	float noiseSize = 4.0f;

	Kernel kernel;
	kernel.kernelSize = 64;
	for (int i = 0; i < kernel.kernelSize; ++i)
	{
		DirectX::SimpleMath::Vector3 v = { rand1(gen) ,rand1(gen) ,rand2(gen) };
		//DirectX::SimpleMath::Vector3 v = {1,0,1};
		v.Normalize();
		v *= pow(rand2(gen), 2);
		kernel.kernel[i] = { v.x, v.y,v.z };
	}
	kernel.scale = { r->getWidth() / noiseSize, r->getHeight() / noiseSize };
	kernel.radius = radius;
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

	std::vector<DirectX::SimpleMath::Vector4> noise(noiseSize * noiseSize);

	for (int i = 0; i < noise.size(); ++i)
	{
		DirectX::SimpleMath::Vector3 d = { rand1(gen), rand1(gen), 0.0f };
		//DirectX::SimpleMath::Vector3 d = { -1,1, 0.0f };

		d.Normalize();
		noise[i] = {d.x, d.y, d.z,0};
	}

	mNoise = r->createTexture( noiseTexDesc, noise.data(), noiseSize * sizeof(DirectX::SimpleMath::Vector4));

	mLinearWrap = r->createSampler("linear_wrap", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPointWrap = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);






}

void AO::render(Renderer::Texture::Ptr rt) 
{
	using namespace DirectX;
	auto cam = getScene()->createOrGetCamera("main");

	ConstantMatrix matrix;

	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();

	XMStoreFloat4x4A(&matrix.invertProjection, proj.Invert().Transpose());
	XMStoreFloat4x4A(&matrix.projection, proj.Transpose());
	XMStoreFloat4x4A(&matrix.view,view.Transpose() );

	mMatrix.lock()->blit(&matrix, sizeof(matrix));


	mQuad->setConstants({ mKernel, mMatrix });
	mQuad->setPixelShader(mPS);
	mQuad->setSamplers({ mLinearWrap ,mPointWrap });
	mQuad->setTextures({ mNormal, mDepth, mNoise});
	
	mQuad->setDefaultBlend();
	mQuad->setRenderTarget(rt);
	mQuad->draw();
}


