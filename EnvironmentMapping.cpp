#include "EnvironmentMapping.h"
#include "GBuffer.h"
#include "PBR.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "MathUtilities.h"


void EnvironmentMapping::init()
{
	mName = "EnvironmentMapping";
	auto renderer = getRenderer();

	std::vector<D3D10_SHADER_MACRO> norm = { {"IBL", "1"},  {NULL,NULL} };
	std::vector<D3D10_SHADER_MACRO> corr = { {"IBL", "1"}, {"CORRECTED", "1"}, {NULL,NULL} };

	mPS[0] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", norm.data());
	mPS[1] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", corr.data());


	mLUT = renderer->createTexture("media/IBL_BRDF_LUT.png", 1);

	mConstants = renderer->createConstantBuffer(sizeof(Constants));
}

void EnvironmentMapping::init(const std::string& cubemap )
{
	init();
	set("envIntensity", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });

	mIsOnlySkybox = true;
	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(), ImageProcessing::RT_COPY, false);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(), ImageProcessing::RT_COPY, false);
	mCube = getRenderer()->createTexture(cubemap, 1);
	mIrradiance = mIrradianceProcessor->process(mCube);
	mPrefiltered = mPrefilteredProcessor->process(mCube);
}


void EnvironmentMapping::init(const Vector3 & pos, const Vector3& size, const std::string& cubemap, int resolution)
{
	init();
	set("envIntensity", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });
	set("envCubeScale", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });
	set("boxProjection", { {"type","set"}, {"value",0},{"min","0"},{"max",1},{"interval", "1"} });

	mIsOnlySkybox = false;

	int cubesize = resolution;
	mSize = size;
	mPosition = pos;
	
	auto cam = getScene()->createOrGetCamera("dynamicEnv");

	auto aabb = getScene()->getRoot()->getWorldAABB();
	Vector3 vec = aabb.second - aabb.first;

	cam->setViewport(0, 0, cubesize, cubesize);
	cam->setNearFar(0.1, vec.Length() + 1); 
	cam->setFOVy(3.14159265358f * 0.5f);

	auto renderer = getRenderer();

	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(), ImageProcessing::RT_COPY, true);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(), ImageProcessing::RT_COPY, true);

	mCubePipeline = decltype(mCubePipeline)(new Pipeline(renderer, getScene()));
	mCubePipeline->setCamera(cam);
	auto w = cubesize;
	auto h = cubesize;
	auto albedo = renderer->createRenderTarget(w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
	mCubePipeline->addShaderResource("albedo", albedo);
	mCubePipeline->addRenderTarget("albedo", albedo);
	auto normal = renderer->createRenderTarget(w, h, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mCubePipeline->addShaderResource("normal", normal);
	mCubePipeline->addRenderTarget("normal", normal);
	auto depth = renderer->createDepthStencil(w, h, DXGI_FORMAT_R24G8_TYPELESS, true);
	mCubePipeline->addShaderResource("depth", depth);
	mCubePipeline->addDepthStencil("depth", depth);
	mCubePipeline->addTexture2D("depth", depth);
	auto material = renderer->createRenderTarget(w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
	mCubePipeline->addShaderResource("material", material);
	mCubePipeline->addRenderTarget("material", material);

	constexpr auto MAX_NUM_LIGHTS = 1024;
	auto pointlights = renderer->createRWBuffer(sizeof(Vector4) * 2 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mCubePipeline->addShaderResource("pointlights", pointlights);
	mCubePipeline->addBuffer("pointlights", pointlights);

	auto spotlights = renderer->createRWBuffer(sizeof(Vector4) * 3 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mCubePipeline->addShaderResource("spotlights", spotlights);
	mCubePipeline->addBuffer("spotlights", spotlights);

	auto dirlights = renderer->createRWBuffer(sizeof(Vector4) * 2 * MAX_NUM_LIGHTS, sizeof(Vector4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	mCubePipeline->addShaderResource("dirlights", dirlights);
	mCubePipeline->addBuffer("dirlights", dirlights);

	{
		Vector4 lightinfo[2];
		auto l = getScene()->createOrGetLight("main");
		auto dir = l->getDirection();
		lightinfo[0] = { dir.x, dir.y,dir.z , 0 };
		auto color = l->getColor() * getValue<float>("dirradiance");
		lightinfo[1] = { color.x, color.y, color.z , 0 };
		dirlights.lock()->blit(lightinfo, sizeof(lightinfo));
	}



	mCubePipeline->pushStage("clear rt", [this, renderer](Renderer::Texture2D::Ptr rt)
	{
		renderer->clearRenderTarget(rt, { 0,0,0,0 });
	});

	mCubePipeline->pushStage<GBuffer>();
	mCubePipeline->pushStage<PBR>();
	if (!cubemap.empty())
	{
		mCubePipeline->pushStage<EnvironmentMapping>(cubemap);
		mCubePipeline->pushStage<SkyBox>(cubemap, false);
	}
	//mCubePipeline->setValue("ambient", 1.0f);
	//mCubePipeline->setValue("numdirs", 1.0f);
	//mCubePipeline->setValue("dirradiance", 1.0f);



	D3D11_TEXTURE2D_DESC cubedesc;
	cubedesc.Width = cubesize;
	cubedesc.Height = cubesize;
	cubedesc.MipLevels = 1;
	cubedesc.ArraySize = 6;
	cubedesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	cubedesc.SampleDesc.Count = 1;
	cubedesc.SampleDesc.Quality = 0;
	cubedesc.Usage = D3D11_USAGE_DEFAULT;
	cubedesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE ;
	cubedesc.CPUAccessFlags = 0;
	cubedesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	mCube = renderer->createTexture(cubedesc);




	auto frame = renderer->createRenderTarget(cubesize, cubesize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mCubePipeline->setFrameBuffer(frame);
	cam->getNode()->setPosition(pos);



	mUpdate = [this,cam, renderer,frame]() {
		int index = 0;
		Matrix viewMats[6] = {
			MathUtilities::makeMatrixFromAxis(-Vector3::UnitZ, Vector3::UnitY, Vector3::UnitX),
			MathUtilities::makeMatrixFromAxis(Vector3::UnitZ, Vector3::UnitY, -Vector3::UnitX),
			MathUtilities::makeMatrixFromAxis(Vector3::UnitX, -Vector3::UnitZ, Vector3::UnitY),
			MathUtilities::makeMatrixFromAxis(Vector3::UnitX, Vector3::UnitZ, -Vector3::UnitY),
			MathUtilities::makeMatrixFromAxis(Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ),
			MathUtilities::makeMatrixFromAxis(-Vector3::UnitX, Vector3::UnitY, -Vector3::UnitZ),
		};

		for (auto& d : viewMats)
		{
			Quaternion rot = Quaternion::CreateFromRotationMatrix(d);
			cam->getNode()->setOrientation(rot);
			mCubePipeline->render();

			renderer->getContext()->CopySubresourceRegion(mCube->getTexture(), index++, 0, 0, 0, frame->getTexture(), 0, 0);
		}

		std::string hdrenvfile = "media/Ditch-River_2k.hdr";
		//mCube = renderer->createTexture(hdrenvfile, 1);

		mIrradiance = mIrradianceProcessor->process(mCube);
		mPrefiltered = mPrefilteredProcessor->process(mCube);

		//D3DX11SaveTextureToFile(renderer->getContext(), mCube->getTexture(), D3DX11_IFF_DDS, L"test.dds");
	};
	
	mUpdate();
}


void EnvironmentMapping::render(Renderer::Texture2D::Ptr rt)
{
	if (mFrame.expired())
	{
		mFrame = getRenderer()->createTexture(rt->getDesc());
	}
	//mUpdate();


	auto cam = getCamera();
	Constants constants;
	constants.invertViewProj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.campos = cam->getNode()->getRealPosition();
	constants.envCubeScale = getValue<float>("envCubeScale");
	constants.envCubeSize = mSize;
	constants.envIntensity = getValue<float>("envIntensity");
	constants.envCubePos = mPosition;
	mConstants.lock()->blit(constants);

	auto quad = getQuad();
	//quad->setDefaultBlend(false);
	quad->setBlendColorAdd();
	quad->setDefaultSampler();
	quad->setViewport(cam->getViewport());
	quad->setPixelShader(mPS[getValue<int>("boxProjection")]);
	quad->setConstant(mConstants);
	quad->setRenderTarget(rt);


	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		getShaderResource("albedo"),
		getShaderResource("normal"),
		getShaderResource("material"),
		getShaderResource("depth"),
	};

	//if (mIsOnlySkybox)
	//	srvs.push_back(mCube);
	//else
	{
		srvs.push_back(mIrradiance);
		srvs.push_back(mPrefiltered);
		srvs.push_back(mLUT);
	}
	quad->setTextures(srvs);
	quad->draw();

	//mFrame->swap(rt);
	//quad->setDefaultPixelShader();
	//quad->setBlendColorAdd();
	//quad->setRenderTarget(rt);
	//quad->setTextures({ mFrame });
	//quad->draw();


}
