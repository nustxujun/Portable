#include "AO.h"
#include <algorithm>
#include <random>
#include "MathUtilities.h"

AO::AO(Renderer::Ptr r, Scene::Ptr s, Quad::Ptr q, Setting::Ptr st, Pipeline* p):
	Pipeline::Stage(r,s,q,st,p)
{
	mName = "AO";

}

void AO::init(float radius)
{
	this->set("AOradius", { {"type","set"}, {"value",radius},{"min","1"},{"max","50"},{"interval", "1"} });
	this->set("AOintensity", { {"type","set"}, {"value",4},{"min","0"},{"max","10"},{"interval", "0.1"} });

	auto r = getRenderer();
	auto blob = r->compileFile("hlsl/ssao.hlsl", "main", "ps_5_0");
	mPS = r->createPixelShader(blob->GetBufferPointer(), blob->GetBufferSize());


	mKernel = r->createBuffer(sizeof(Kernel), D3D11_BIND_CONSTANT_BUFFER);
	mMatrix = r->createBuffer(sizeof(ConstantMatrix), D3D11_BIND_CONSTANT_BUFFER);

	std::uniform_real_distribution<float> rand1(-1.0, 1.0);
	std::uniform_real_distribution<float> rand2(0.0, 1.0);
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
		v *= pow(rand2(gen), 4);
		kernel.kernel[i] = { v.x, v.y,v.z ,0};
	}
	kernel.scale = { r->getWidth() / noiseSize, r->getHeight() / noiseSize };
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
		noise[i] = { d.x, d.y, d.z,0 };
	}

	mNoise = r->createTexture(noiseTexDesc);
	mNoise->blit(noise.data(), noiseSize * sizeof(DirectX::SimpleMath::Vector4));

	mLinearWrap = r->createSampler("linear_wrap", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	mPointWrap = r->createSampler("point_wrap", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	mGaussianFilter = ImageProcessing::create<Gaussian>(getRenderer());
}


void AO::render(Renderer::Texture2D::Ptr rt) 
{
	if (mAO.expired())
		mAO = getRenderer()->createTexture(rt.lock()->getDesc());

	using namespace DirectX;
	auto cam = getCamera();

	ConstantMatrix matrix;
	matrix.radius = getValue<float>("AOradius");
	matrix.intensity = getValue<float>("AOintensity");

	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();

	XMStoreFloat4x4A(&matrix.invertProjection, proj.Invert().Transpose());
	XMStoreFloat4x4A(&matrix.projection, proj.Transpose());
	XMStoreFloat4x4A(&matrix.view,view.Transpose() );

	mMatrix.lock()->blit(&matrix, sizeof(matrix));

	auto quad = getQuad();
	quad->setConstants({ mKernel, mMatrix });
	quad->setPixelShader(mPS);
	quad->setSamplers({ mLinearWrap ,mPointWrap });
	quad->setTextures({ getShaderResource("normal"), getShaderResource("depth"), mNoise});
	
	quad->setDefaultBlend(false);
	quad->setRenderTarget(mAO);
	quad->draw();

	auto ret = mGaussianFilter->process(mAO);
	quad->setBlendColorMul();
	quad->setRenderTarget(rt);
	quad->setDefaultPixelShader();
	quad->setDefaultSampler();
	quad->setDefaultViewport();
	quad->setTextures({ ret->get() });
	quad->draw();
}


