#include "EnvironmentMapping.h"
#include "GBuffer.h"
#include "PBR.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "MathUtilities.h"
#include "SphericalHarmonics.h"
#include "DepthLinearing.h"

static auto constexpr MAX_NUM_PROBES = 10;


void EnvironmentMapping::init()
{

	setValue("envmap", true);
	mName = "EnvironmentMapping";
	auto renderer = getRenderer();

	std::vector<D3D10_SHADER_MACRO> norm = {   {NULL,NULL} };
	std::vector<D3D10_SHADER_MACRO> corr = {  {"CORRECTED", "1"}, {NULL,NULL} };


	for (int i = 0; i < 3; ++i)
	{
		std::vector<D3D10_SHADER_MACRO> macros;
		switch (i)
		{
		case 0: break;
		case 1: macros.push_back({ "SH", "1" }); break;
		case 2: macros.push_back({ "IBL", "1" }); break;
		default:
			break;
		}

		macros.push_back({ NULL,NULL });
		mPS[i][0] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", macros.data());
		macros.insert(macros.end() - 1, { "CORRECTED", "1" });
		mPS[i][1] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", macros.data());
		((macros.end() - 2))->Name = "DEPTH_CORRECTED";
		mPS[i][2] = renderer->createPixelShader("hlsl/environmentmapping.hlsl", "main", macros.data());
	}

	mLinear = getRenderer()->createSampler("linear_clamp", D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	mPoint = getRenderer()->createSampler("point_clamp", D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);



	mLUT = renderer->createTexture("media/IBL_BRDF_LUT.png", 1);

	mConstants = renderer->createConstantBuffer(sizeof(Constants));

	//mProbeInfos = renderer->createRWBuffer(sizeof(Vector3) * 6 * MAX_NUM_PROBES, sizeof(Vector3), DXGI_FORMAT_R32G32B32_FLOAT, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void EnvironmentMapping::init(const std::string& cubemap )
{
	init();
	set("envIntensity", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });

	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(), false);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(), false);
	mCube = getRenderer()->createTexture(cubemap, 1);
	mIrradiance.push_back( mIrradianceProcessor->process(mCube));
	mPrefiltered.push_back( mPrefilteredProcessor->process(mCube));

	//auto aabb = getScene()->getRoot()->getWorldAABB();
	//Vector3 raw[6] = { (aabb.first + aabb.second)* 0.5f, aabb.first , aabb.second,  aabb.first, aabb.second , {1,1,1} };

	//mProbeInfos.lock()->blit(raw, sizeof(raw));
}


void EnvironmentMapping::init(Type type, const std::string& cubemap, int resolution)
{
	mType = type;
	init();
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

	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(),  true);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(),   true);

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
	auto depthlinear = renderer->createRenderTarget(w, h, DXGI_FORMAT_R32_FLOAT);
	mCubePipeline->addShaderResource("depthlinear", depthlinear);
	mCubePipeline->addRenderTarget("depthlinear", depthlinear);
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

	mCubePipeline->pushStage<GBuffer>([](Scene::Entity::Ptr e)->bool
	{
		return e->isStatic();
	});

	mCubePipeline->pushStage<DepthLinearing>();

	mCubePipeline->pushStage<PBR>();
	if (!cubemap.empty())
	{
		//mCubePipeline->pushStage<EnvironmentMapping>(cubemap);
		mCubePipeline->pushStage<SkyBox>(cubemap, false);
	}
	mCubePipeline->setValue("ambient", 1.0f);
	//mCubePipeline->setValue("numdirs", 1.0f);
	//mCubePipeline->setValue("dirradiance", 1.0f);



	auto frame = renderer->createRenderTarget(cubesize, cubesize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mCubePipeline->setFrameBuffer(frame);

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
	mCube = renderer->createTexture(cubedesc);

	mUpdate = [this,cam, renderer,frame, depthlinear](bool force) {
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
			mIrradiance.clear();
			mPrefiltered.clear();
			mCoefficients.clear();
			for (auto& p : probes)
			{
				auto desc = depthlinear->getDesc();
				desc.ArraySize = 6;
				desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
				mDepthCorrected.push_back(renderer->createTemporaryRT(desc));

				auto pos = p.second->getNode()->getRealPosition();


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
					renderer->getContext()->CopySubresourceRegion(mCube->getTexture(), index, 0, 0, 0, frame->getTexture(), 0, 0);
					renderer->getContext()->CopySubresourceRegion(mDepthCorrected[nums]->get()->getTexture(), index, 0, 0, 0, depthlinear->getTexture(), 0, 0);
					index++;

				}
				if (p.second->getType() == Scene::Probe::PT_IBL)
				{
					mIrradiance.push_back(mIrradianceProcessor->process(mCube));
					mPrefiltered.push_back(mPrefilteredProcessor->process(mCube));
					//D3DX11SaveTextureToFile(getRenderer()->getContext(), mCube->getTexture(), D3DX11_IFF_DDS, L"test.dds");

				}
				else if (p.second->getType() == Scene::Probe::PT_DIFFUSE)
				{
					mIrradiance.push_back({});
					mPrefiltered.push_back({});


					auto constexpr degree = 1;
					auto constexpr num_coefs = (degree + 1) * (degree + 1);
					std::vector<Vector3> coefs(num_coefs);

					//float r[num_coefs] = { 0 };
					//float g[num_coefs] = { 0 };
					//float b[num_coefs] = { 0 };
					//D3DX11SHProjectCubeMap(getRenderer()->getContext(), degree + 1 , mCube->getTexture(), r, g, b);

					//for (int i = 0; i < num_coefs; ++i)
					//{
					//	coefs[i] = { r[i], g[i],b[i] };
					//}
					coefs = SphericalHarmonics::convert<degree>(mCube, getRenderer());

					if (mCoefficients.size() <= nums)
					{
						mCoefficients.push_back(getRenderer()->createRWBuffer(sizeof(Vector3) * num_coefs, sizeof(Vector3), DXGI_FORMAT_R32G32B32_FLOAT,D3D11_BIND_SHADER_RESOURCE,D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITE));
					}
					mCoefficients[nums]->blit(coefs.data(), sizeof(Vector3) * num_coefs);
				}
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
	auto vp = cam->getViewport();
	auto view = cam->getViewMatrix();
	auto proj = cam->getProjectionMatrix();
	
	Constants constants;
	constants.proj = proj.Transpose();
	constants.view = view.Transpose();
	constants.invertView = view.Invert().Transpose();
	constants.invertProj = proj.Invert().Transpose();
	constants.invertViewProj = (view * proj).Invert().Transpose();

	constants.campos = cam->getNode()->getRealPosition();
	constants.nearZ = cam->getNear();

	constants.raylength = 1000;
	
	constants.intensity = { 1,1,1 };
	constants.stepstride = 4;
	constants.screenSize = { vp.Width, vp.Height };


	int selected = 0;
	auto quad = getQuad();
	if (has("ssr"))
	{
		getRenderer()->clearRenderTarget(mFrame, { 0,0,0,0 });
		quad->setRenderTarget(mFrame);
	}
	else
		quad->setRenderTarget(rt);

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

	if (!probe )
		return;

	auto node = probe->getNode();
	auto pos = node->getRealPosition();
	constants.probepos = { pos.x, pos.y, pos.z };
	auto box = probe->getProxyBox();
	constants.boxmin = { pos + box.second - box.first * 0.5f };
	constants.boxmax = { pos + box.second + box.first * 0.5f };
	auto inf = probe->getInfluence();
	constants.infmin = { pos + inf.second - inf.first * 0.5f };
	constants.infmax = { pos + inf.second + inf.first * 0.5f };

	mConstants.lock()->blit(constants);

	//quad->setDefaultBlend(false);
	quad->setBlendColorAdd();
	quad->setSamplers({ mLinear, mPoint });
	quad->setViewport(vp);
	quad->setPixelShader( mPS[probe->getType()][probe->getProxy()]);
	quad->setConstant(mConstants);




	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		getShaderResource("albedo"),
		getShaderResource("normal"),
		getShaderResource("material"),
		getShaderResource("depth"),
		mDepthCorrected[selected]->get(),
	};

	if (probe->getType() == Scene::Probe::PT_IBL)
	{
		srvs.push_back(mLUT);
		srvs.push_back(mIrradiance[selected]->get());
		srvs.push_back(mPrefiltered[selected]->get());
	}
	else if (probe->getType() == Scene::Probe::PT_DIFFUSE)
	{
		srvs.push_back(mCoefficients[selected]);
	}
	else
		srvs.push_back(mCube);

	quad->setTextures(srvs);
	quad->draw();

	//mFrame->swap(rt);
	//quad->setDefaultPixelShader();
	//quad->setBlendColorAdd();
	//quad->setRenderTarget(rt);
	//quad->setTextures({ mFrame });
	//quad->draw();


}
