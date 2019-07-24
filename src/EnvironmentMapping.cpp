#include "EnvironmentMapping.h"
#include "GBuffer.h"
#include "PBR.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "MathUtilities.h"
#include "SphericalHarmonics.h"

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
		case 1: macros.push_back({ "IBL", "1" }); break;
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

	mCalDistanceConst = renderer->createConstantBuffer(sizeof(Matrix));
	mCalDistance = renderer->createPixelShader("hlsl/distance.hlsl");
}

Renderer::TemporaryRT::Ptr EnvironmentMapping::calDistance(Renderer::Texture2D::Ptr rt, const Matrix& inverViewProj)
{
	mCalDistanceConst->map();
	mCalDistanceConst->write(inverViewProj);
	mCalDistanceConst->unmap();

	auto quad = getQuad();
	auto desc = rt->getDesc();
	desc.Format = DXGI_FORMAT_R32_FLOAT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	auto tmp = getRenderer()->createTemporaryRT(desc);

	quad->setViewport({ 0.0f, 0.0f, (float)desc.Width, (float)desc.Height, 0.0f, 0.1f });
	quad->setDefaultBlend(false);
	quad->setDefaultSampler();

	quad->setTextures({ rt });
	quad->setRenderTarget(tmp->get());
	quad->setPixelShader(mCalDistance);
	quad->setConstant(mCalDistanceConst);
	quad->draw();

	return tmp;
}

void EnvironmentMapping::init(const std::string& cubemap )
{
	init();
	set("envIntensity", { {"type","set"}, {"value",1},{"min","0"},{"max",2.0f},{"interval", "0.01"} });

	mIrradianceProcessor = ImageProcessing::create<IrradianceCubemap>(getRenderer(), false);
	mPrefilteredProcessor = ImageProcessing::create<PrefilterCubemap>(getRenderer(), false);


	auto tex = getRenderer()->createTexture(cubemap, 1);

	mCube.push_back(getRenderer()->createTemporaryRT(tex->getDesc()));
	getRenderer()->getContext()->CopyResource(mCube[0]->get()->getTexture(), tex->getTexture());
	getRenderer()->destroyTexture(tex);


	mIrradiance.push_back( mIrradianceProcessor->process(mCube[0]->get()));
	mPrefiltered.push_back( mPrefilteredProcessor->process(mCube[0]->get()));

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



	mCubePipeline->pushStage("clear rt", [this, renderer](Renderer::Texture2D::Ptr rt)
	{
		renderer->clearRenderTarget(rt, { 0,0,0,0 });
	});

	mCubePipeline->pushStage<GBuffer>(true,[this](Scene::Entity::Ptr e)->bool
	{
		if (mType == T_ONCE)
			return e->isStatic();
		else
			return true;
	});

	mCubePipeline->pushStage<PBR>();
	if (!cubemap.empty())
	{
		//mCubePipeline->pushStage<EnvironmentMapping>(cubemap);
		mCubePipeline->pushStage<SkyBox>(cubemap, false);
	}
	//mCubePipeline->setValue("ambient", 1.0f);
	//mCubePipeline->setValue("numdirs", 1.0f);
	//mCubePipeline->setValue("dirradiance", 1.0f);

	mCubePipeline->setValue("dirradiance", getValue<float>("dirradiance"));
	mCubePipeline->setValue("pointradiance", getValue<float>("pointradiance"));
	//mCubePipeline->setValue("lightRange", vec.Length());


	int numdirs = 0;
	int numpoints = 0;

	getScene()->visitLights([&numdirs, &numpoints](Scene::Light::Ptr l)
	{
		switch (l->getType())
		{
		case Scene::Light::LT_DIR: numdirs++; break;
		case Scene::Light::LT_POINT: numpoints++; break;
		}
	});

	mCubePipeline->setValue("numdirs", numdirs);
	mCubePipeline->setValue("numpoints", numpoints);



	auto frame = renderer->createRenderTarget(cubesize, cubesize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mCubePipeline->setFrameBuffer(frame);


	auto depth = mCubePipeline->getTexture2D("depth");
	mUpdate = [this,cam, renderer,frame,depth, cubesize](bool force) {
		auto scene = getScene();
		auto campos = scene->createOrGetCamera("main")->getNode()->getRealPosition();
		auto probes = scene->getProbes();
		std::vector<Vector3> raw;
		//mNumProbes = 0;
		float sumdist = 0;
		float mindist = FLT_MAX;
		std::vector<float> dists;
		int nums = 0;

		int miplevels = 10;
		if (force || mType == T_EVERYFRAME)
		{
			mIrradiance.clear();
			mPrefiltered.clear();
			mDepthCorrected.clear();
			mCube.clear();
			for (auto& p : probes)
			{
				{
					D3D11_TEXTURE2D_DESC cubedesc;
					cubedesc.Width = cubesize;
					cubedesc.Height = cubesize;
					cubedesc.MipLevels = miplevels;
					cubedesc.ArraySize = 6 ;
					cubedesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					cubedesc.SampleDesc.Count = 1;
					cubedesc.SampleDesc.Quality = 0;
					cubedesc.Usage = D3D11_USAGE_DEFAULT;
					cubedesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
					cubedesc.CPUAccessFlags = 0;
					cubedesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
					mCube.push_back( renderer->createTemporaryRT(cubedesc));
				}
				{
					auto desc = depth->getDesc();
					desc.ArraySize = 6;
					desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
					desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					desc.Format = DXGI_FORMAT_R32_FLOAT;
					mDepthCorrected.push_back(renderer->createTemporaryRT(desc));
				}
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

					auto cubeindex = D3D10CalcSubresource(0, index, miplevels);

					renderer->getContext()->CopySubresourceRegion(mCube[nums]->get()->getTexture(), cubeindex, 0, 0, 0, frame->getTexture(), 0, 0);

					auto proj = cam->getProjectionMatrix();

					auto dist = calDistance(depth,  proj.Invert().Transpose());
					//D3DX11SaveTextureToFile(getRenderer()->getContext(), mCube[nums]->getTexture(), D3DX11_IFF_DDS, L"test.dds");

					
					renderer->getContext()->CopySubresourceRegion(mDepthCorrected[nums]->get()->getTexture(), index, 0, 0, 0, dist->get()->getTexture(), 0, 0);
					index++;

				}
				if (p.second->getType() == Scene::Probe::PT_IBL)
				{
					mIrradiance.push_back(mIrradianceProcessor->process(mCube[nums]->get()));
					mPrefiltered.push_back(mPrefilteredProcessor->process(mCube[nums]->get()));

				}
				else
				{
					renderer->getContext()->GenerateMips(mCube[nums]->get()->Renderer::ShaderResource::getView());
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
	constants.invertViewProj = (view * proj).Invert().Transpose();

	constants.campos = cam->getNode()->getRealPosition();
	constants.nearZ = cam->getNear();

	constants.maxsteps = 100000;
	
	constants.intensity = { 1,1,1 };
	constants.stepsize = 1;
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


	Renderer::ShaderResource::Ptr depthCorrected;
	if (probe->getProxy() == Scene::Probe::PP_DEPTH)
		depthCorrected = mDepthCorrected[selected]->get();

	std::vector<Renderer::ShaderResource::Ptr> srvs = {
		getShaderResource("albedo"),
		getShaderResource("normal"),
		getShaderResource("material"),
		getShaderResource("depth"),
		depthCorrected,
		mLUT
	};

	if (probe->getType() == Scene::Probe::PT_IBL)
	{
		srvs.push_back(mIrradiance[selected]->get());
		srvs.push_back(mPrefiltered[selected]->get());
	}
	else
		srvs.push_back(mCube[selected]->get());

	quad->setTextures(srvs);
	quad->draw();

	//mFrame->swap(rt);
	//quad->setDefaultPixelShader();
	//quad->setBlendColorAdd();
	//quad->setRenderTarget(rt);
	//quad->setTextures({ mFrame });
	//quad->draw();


}
