#include "EnvironmentMapping.h"
#include "GBuffer.h"
#include "PBR.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "MathUtilities.h"


static auto constexpr MAX_NUM_PROBES = 10;


void EnvironmentMapping::init(bool ibl)
{
	mIBL = ibl;
	setValue("envmap", true);
	mName = "EnvironmentMapping";
	auto renderer = getRenderer();

	std::vector<D3D10_SHADER_MACRO> norm = {   {NULL,NULL} };
	std::vector<D3D10_SHADER_MACRO> corr = {  {"CORRECTED", "1"}, {NULL,NULL} };

	if (ibl)
	{
		norm.insert(norm.begin(), { "IBL", "1" });
		corr.insert(corr.begin(), { "IBL", "1" });
	}

	mPS[0] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", norm.data());
	mPS[1] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", corr.data());


	mLUT = renderer->createTexture("media/IBL_BRDF_LUT.png", 1);

	mConstants = renderer->createConstantBuffer(sizeof(Constants));

	//mProbeInfos = renderer->createRWBuffer(sizeof(Vector3) * 6 * MAX_NUM_PROBES, sizeof(Vector3), DXGI_FORMAT_R32G32B32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void EnvironmentMapping::init(const std::string& cubemap )
{
	init(true);
	set("envIntensity", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });

	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(), ImageProcessing::RT_COPY, false);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(), ImageProcessing::RT_COPY, false);
	mCube.push_back(getRenderer()->createTexture(cubemap, 1));
	mIrradiance.push_back( mIrradianceProcessor->process(mCube[0]));
	mPrefiltered.push_back( mPrefilteredProcessor->process(mCube[0]));

	mSkyOnly = true;
	//auto aabb = getScene()->getRoot()->getWorldAABB();
	//Vector3 raw[6] = { (aabb.first + aabb.second)* 0.5f, aabb.first , aabb.second,  aabb.first, aabb.second , {1,1,1} };

	//mProbeInfos.lock()->blit(raw, sizeof(raw));
}


void EnvironmentMapping::init(Type type, const std::string& cubemap, int resolution)
{
	mType = type;
	init(true);
	set("envIntensity", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });
	set("boxProjection", { {"type","set"}, {"value",1},{"min","0"},{"max",1},{"interval", "1"} });




	int cubesize = resolution;
	
	auto cam = getScene()->createOrGetCamera("dynamicEnv");

	auto aabb = getScene()->getRoot()->getWorldAABB();
	Vector3 vec = aabb.second - aabb.first;

	cam->setViewport(0, 0, cubesize, cubesize);
	cam->setNearFar(0.1, vec.Length() + 1); 
	cam->setFOVy(3.14159265358f * 0.5f);

	auto renderer = getRenderer();

	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(), mType == T_EVERYFRAME? ImageProcessing::RT_TEMP: ImageProcessing::RT_COPY, true);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(), mType == T_EVERYFRAME ? ImageProcessing::RT_TEMP : ImageProcessing::RT_COPY,  true);

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



	mCubePipeline->pushStage("clear rt", [this, renderer,depth](Renderer::Texture2D::Ptr rt)
	{
		renderer->clearRenderTarget(rt, { 0,0,0,0 });
		renderer->clearDepth(depth, 1.0f);
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



	auto frame = renderer->createRenderTarget(cubesize, cubesize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mCubePipeline->setFrameBuffer(frame);

	mUpdate = [this,cam, renderer,frame, cubesize](bool force) {
		auto scene = getScene();
		auto campos = scene->createOrGetCamera("main")->getNode()->getRealPosition();
		auto probes = scene->getProbes();
		std::vector<Vector3> raw;
		//mNumProbes = 0;
		float sumdist = 0;
		float mindist = FLT_MAX;
		std::vector<float> dists;
		int nums = 0;
		if (force || mType == T_EVERYFRAME)
		{
			mCube.clear();
			mIrradiance.clear();
			mPrefiltered.clear();
			for (auto& p : probes)
			{
				auto pos = p.second->getNode()->getRealPosition();
				D3D11_TEXTURE2D_DESC cubedesc;
				cubedesc.Width = cubesize;
				cubedesc.Height = cubesize;
				cubedesc.MipLevels = 1;
				cubedesc.ArraySize = 6;
				cubedesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				cubedesc.SampleDesc.Count = 1;
				cubedesc.SampleDesc.Quality = 0;
				cubedesc.Usage = D3D11_USAGE_DEFAULT;
				cubedesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				cubedesc.CPUAccessFlags = 0;
				cubedesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
				mCube.push_back(renderer->createTexture(cubedesc));

				cam->getNode()->setPosition(pos);
				Matrix viewMats[6] = {
					MathUtilities::makeMatrixFromAxis(-Vector3::UnitZ, Vector3::UnitY, Vector3::UnitX),
					MathUtilities::makeMatrixFromAxis(Vector3::UnitZ, Vector3::UnitY, -Vector3::UnitX),
					MathUtilities::makeMatrixFromAxis(Vector3::UnitX, -Vector3::UnitZ, Vector3::UnitY),
					MathUtilities::makeMatrixFromAxis(Vector3::UnitX, Vector3::UnitZ, -Vector3::UnitY),
					MathUtilities::makeMatrixFromAxis(Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ),
					MathUtilities::makeMatrixFromAxis(-Vector3::UnitX, Vector3::UnitY, -Vector3::UnitZ),
				};
				int index = 0;
				for (auto& d : viewMats)
				{
					Quaternion rot = Quaternion::CreateFromRotationMatrix(d);
					cam->getNode()->setOrientation(rot);
					mCubePipeline->render();
					renderer->getContext()->CopySubresourceRegion(mCube[nums]->getTexture(), index++, 0, 0, 0, frame->getTexture(), 0, 0);
				}

				mIrradiance.push_back(mIrradianceProcessor->process(mCube[nums]));
				mPrefiltered.push_back(mPrefilteredProcessor->process(mCube[nums]));
				nums++;

			}
		}
	

	};
	
	mUpdate(true);
}


void EnvironmentMapping::render(Renderer::Texture2D::Ptr rt)
{
	if (mFrame.expired())
	{
		mFrame = getRenderer()->createTexture(rt->getDesc());
		addShaderResource("envmap", mFrame);
	}
	if (mUpdate)
		mUpdate(false);


	auto cam = getCamera();
	Constants constants;
	constants.invertViewProj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	constants.campos = cam->getNode()->getRealPosition();
	constants.intensity = { 1,1,1 };
	int selected = 0;
	auto quad = getQuad();
	if (has("ssr"))
	{
		getRenderer()->clearRenderTarget(mFrame, { 0,0,0,0 });
		quad->setRenderTarget(mFrame);
	}
	else
		quad->setRenderTarget(rt);

	if (!mSkyOnly)
	{
		auto probes = getScene()->getProbes();

		int index = 0;
		Scene::Probe::Ptr probe;
		float mindist = FLT_MAX;
		for (auto& p : probes)
		{
			auto pos = p.second->getNode()->getPosition();
			float d = (constants.campos - pos).LengthSquared();
			if (d < mindist 
				//&& p.second->intersect(constants.campos)
				)
			{
				mindist = d;
				probe = p.second;
				selected = index;
			}

			index++;
		}

		if (!probe)
			return;

		auto node = probe->getNode();
		auto pos = node->getRealPosition();
		constants.probepos = { pos.x, pos.y, pos.z };
		auto box = probe->getProjectionBox();
		constants.boxmin = { pos + box.second - box.first * 0.5f };
		constants.boxmax = { pos + box.second + box.first * 0.5f };
		auto inf = probe->getInfluence();
		constants.infmin = { pos + inf.second - inf.first * 0.5f };
		constants.infmax = { pos + inf.second + inf.first * 0.5f };
	}

	mConstants.lock()->blit(constants);

	//quad->setDefaultBlend(false);
	quad->setBlendColorAdd();
	quad->setDefaultSampler();
	quad->setViewport(cam->getViewport());
	quad->setPixelShader(mSkyOnly ? mPS[0] : mPS[getValue<int>("boxProjection")]);
	quad->setConstant(mConstants);




	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		getShaderResource("albedo"),
		getShaderResource("normal"),
		getShaderResource("material"),
		getShaderResource("depth"),
		//mLUT,
	};

	if (mIBL)
	{
		srvs.push_back(mLUT);
		srvs.push_back(mIrradiance[selected]);
		srvs.push_back(mPrefiltered[selected]);
	}
	else
		srvs.push_back(mCube[selected]);

	quad->setTextures(srvs);
	quad->draw();

	//mFrame->swap(rt);
	//quad->setDefaultPixelShader();
	//quad->setBlendColorAdd();
	//quad->setRenderTarget(rt);
	//quad->setTextures({ mFrame });
	//quad->draw();


}