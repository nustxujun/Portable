#include "IrradianceVolumes.h"
#include "GBuffer.h"
#include "PBR.h"
#include "PostProcessing.h"
#include "SkyBox.h"
#include "MathUtilities.h"
#include "SphericalHarmonics.h"
#include "ShadowMap.h"
#include <thread>
#include <iostream>

const size_t IrradianceVolumes::SH_DEGREE = 2;
const size_t IrradianceVolumes::NUM_COEFS = (IrradianceVolumes::SH_DEGREE + 1) * (IrradianceVolumes::SH_DEGREE + 1);
const size_t IrradianceVolumes::COEFS_ARRAY_SIZE = ALIGN(NUM_COEFS * 3, 4)  / 4;
const size_t IrradianceVolumes::RESOLUTION = 128;

#define SHOW_DEBUG_OBJECT 0

void IrradianceVolumes::init(const Vector3 & size, const std::string& sky)
{
	mName = "irradiance volumes";

	set("IVintensity", { {"type","set"}, {"value",1},{"min","0"},{"max","10"},{"interval", "0.1"} });

	auto scene = getScene();
	auto renderer = getRenderer();

	mSize = size ;

	for (size_t i = 0; i < COEFS_ARRAY_SIZE; ++i)
	{
		D3D11_TEXTURE3D_DESC texdesc;
		texdesc.Width = mSize.x;
		texdesc.Height = mSize.y;
		texdesc.Depth = mSize.z;
		texdesc.MipLevels = 1;
		texdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texdesc.Usage = D3D11_USAGE_DYNAMIC;
		texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texdesc.MiscFlags = 0;
		mCoefficients.push_back( renderer->createTexture3D(texdesc) );
	}


	auto tostring = [](auto v)
	{
		std::stringstream ss;
		ss << v;
		return std::string(ss.str());
	};

	auto degree = tostring(SH_DEGREE);
	auto arraysize = tostring(COEFS_ARRAY_SIZE);

	std::vector<D3D10_SHADER_MACRO> macros = {
		{"DEGREE",degree.c_str()},
		{"COEF_ARRAY_SIZE", arraysize.c_str()},
		{NULL, NULL}
	};
	mPS = renderer->createPixelShader("hlsl/irradiancevolumes.hlsl","main", macros.data());
	mConstants = renderer->createConstantBuffer(sizeof(Constants));
	initBakedPipeline(sky);
}

void IrradianceVolumes::initBakedPipeline(const std::string& sky)
{

	int cubesize = RESOLUTION;

	auto cam = getScene()->createOrGetCamera("dynamicEnv");

	auto aabb = getScene()->getRoot()->getWorldAABB();
	Vector3 vec = aabb.second - aabb.first;

	cam->setViewport(0, 0, cubesize, cubesize);
	cam->setNearFar(0.1, vec.Length() + 1);
	cam->setFOVy(3.14159265358f * 0.5f);

	auto renderer = getRenderer();

	constexpr auto MAX_SHADOWMAPS = 1;
	std::vector<Renderer::Texture2D::Ptr> shadowmaps;
	for (int i = 0; i < MAX_SHADOWMAPS; ++i)
		shadowmaps.push_back(renderer->createRenderTarget(cubesize, cubesize, DXGI_FORMAT_R32_FLOAT));


	mBakedPipeline = decltype(mBakedPipeline)(new Pipeline(renderer, getScene()));
	mBakedPipeline->setCamera(cam);
	auto w = cubesize;
	auto h = cubesize;


	mBakedPipeline->pushStage("clear rt", [renderer](Renderer::Texture2D::Ptr rt) {
		renderer->clearRenderTarget(rt, { 0,0,0,0, });
	});
	mBakedPipeline->pushStage<GBuffer>(true, [](Scene::Entity::Ptr e)->bool
	{
		return e->isStatic();
	});
	mBakedPipeline->pushStage<ShadowMap>(cubesize, 1, shadowmaps);
	mBakedPipeline->pushStage<PBR>(Vector3(), shadowmaps);
	mBakedPipeline->pushStage<SkyBox>(sky, false);

	//mBakedPipeline->setValue("ambient", 1.0f);
	mBakedPipeline->setValue("dirradiance", 1);
	mBakedPipeline->setValue("pointradiance", 1);
	mBakedPipeline->setValue("lightRange", vec.Length());

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

	mBakedPipeline->setValue("numdirs", numdirs);
	mBakedPipeline->setValue("numpoints", numpoints);



	auto frame = renderer->createRenderTarget(cubesize, cubesize, DXGI_FORMAT_R32G32B32A32_FLOAT);
	mBakedPipeline->setFrameBuffer(frame);

	std::cout << "computing sphecial harmonics coefficients ..." << std::endl;

	mCal = [cubesize, frame,this]() {
		calCoefs(cubesize, frame);
	};
	mCal();
}

void IrradianceVolumes::calCoefs(size_t cubesize, Renderer::Texture2D::Ptr frame)
{
	auto scene = getScene();
	auto renderer = getRenderer();
	auto context = renderer->getContext();
	auto cam = scene->createOrGetCamera("dynamicEnv");
	auto aabb = getScene()->getRoot()->getWorldAABB();
	auto vec = aabb.second - aabb.first;

	auto len = vec / (mSize );
	auto half = len * 0.5f;

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
	auto cube = renderer->createTemporaryRT(cubedesc);

	Matrix rots[6] = {
		MathUtilities::makeMatrixFromAxis(-Vector3::UnitZ, Vector3::UnitY, Vector3::UnitX),
		MathUtilities::makeMatrixFromAxis(Vector3::UnitZ, Vector3::UnitY, -Vector3::UnitX),
		MathUtilities::makeMatrixFromAxis(Vector3::UnitX, -Vector3::UnitZ, Vector3::UnitY),
		MathUtilities::makeMatrixFromAxis(Vector3::UnitX, Vector3::UnitZ, -Vector3::UnitY),
		MathUtilities::makeMatrixFromAxis(Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ),
		MathUtilities::makeMatrixFromAxis(-Vector3::UnitX, Vector3::UnitY, -Vector3::UnitZ),
	};


	std::array<std::vector<Vector4>, COEFS_ARRAY_SIZE> coefs;
	for (auto & c : coefs)
	{
		c.resize(mSize.x * mSize.y* mSize.z);
	}

#if SHOW_DEBUG_OBJECT
	Parameters params;
	params["geom"] = "sphere";
	params["radius"] = Common::format(vec.Length() * 0.01f * std::pow(1 / mSize.Length(), 0.5f));
	auto sphere = Mesh::Ptr(new GeometryMesh(params, renderer));
	sphere->getMesh(0).material->roughness = 1;
	sphere->getMesh(0).material->metallic = 0;
#endif

	std::list<std::thread> threads;

	for (float x = 0; x < mSize.x; ++x)
	{
		for (float y = 0; y < mSize.y; ++y)
		{
			for (float z = 0; z < mSize.z; ++z)
			{
				// capture the faces of cubemap
				for (int i = 0; i < ARRAYSIZE(rots); ++i)
				{
					Quaternion rot = Quaternion::CreateFromRotationMatrix(rots[i]);
					auto node = cam->getNode();
					node->setOrientation(rot);
					node->setPosition(aabb.first  + len * Vector3(x, y, z) + half);

#if SHOW_DEBUG_OBJECT
					auto model = scene->createModel(Common::format("light_probe", x, y, z), params, [sphere](const Parameters& p) {
						return sphere;
					});
					model->getNode()->setPosition(aabb.first  + len * Vector3(x, y, z) + half);
					model->attach(scene->getRoot());
					model->setStatic(false);
#endif

					mBakedPipeline->render();
					context->CopySubresourceRegion(cube->get()->getTexture(), i, 0, 0, 0, frame->getTexture(), 0, 0);
				}

				//D3DX11SaveTextureToFileA(context, cube->get()->getTexture(), D3DX11_IFF_DDS, "test.dds");

				using TEXCUBE = std::array<std::vector<Vector4>, 6>;
				std::shared_ptr<TEXCUBE> content =  decltype(content)(new TEXCUBE());
				readTextureCube(cube->get(), *content);
				
				auto maxthreads = std::thread::hardware_concurrency() - 1; 
				if (threads.size() > maxthreads)
				{
					threads.begin()->join();
					threads.pop_front();
				}
				threads.emplace_back([=,&coefs]()
				{
					std::vector<Vector3> ret = SphericalHarmonics::precompute< SH_DEGREE>(*content, cubesize);
					size_t index = (size_t)x + (size_t)y * mSize.x + (size_t)z * mSize.x * mSize.y;

					const float* beg = (const float*)ret.data();
					const float* end = (const float*)(ret.data() + ret.size());
					for (int i = 0; i < coefs.size(); ++i)
					{
						float* v = (float*)(&coefs[i][index]);
						for (int j = 0; j < 4; ++j)
						{
							if (beg == end)
								break;
							*(v++) = *(beg++);
						}
					}
				
				});

			}
		}
	}

	for (auto& t : threads)
		t.join();

	
	for (int i = 0; i < mCoefficients.size(); ++i)
	{
		auto tex = mCoefficients[i];
		D3D11_TEXTURE3D_DESC desc;
		tex->getTexture()->GetDesc(&desc);
		D3D11_MAPPED_SUBRESOURCE subres;
		context->Map(tex->getTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
		char* data;
		char* head = (char*)subres.pData;

		auto coef = coefs[i].data();
		for (int z = 0; z < mSize.z; ++z)
		{
			for (int y = 0; y < mSize.y; ++y)
			{
				data = head + subres.RowPitch * y +  subres.DepthPitch * z;
				memcpy(data, coef + ((int)mSize.x * y) + ((int)mSize.x * (int)mSize.y * z), sizeof(Vector4) * mSize.x);
			}
		}
		context->Unmap(tex->getTexture(), 0);
	}
}

void IrradianceVolumes::readTextureCube(Renderer::Texture2D::Ptr cube, std::array<std::vector<Vector4>, 6>& contents)
{
	auto renderer = getRenderer();
	// create staging texture for cpu reading
	auto desc = cube->getDesc();
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = 0;
	desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	auto temp = renderer->createTemporaryRT(desc);
	auto tex = temp->get();
	renderer->getContext()->CopyResource(tex->getTexture(), cube->getTexture());

	auto context = renderer->getContext();
	D3D11_MAPPED_SUBRESOURCE subresource;
	for (int face = 0; face < 6; ++face)
	{
		auto index = D3D10CalcSubresource(0, face, desc.MipLevels);
		HRESULT hr = context->Map(tex->getTexture(), index, D3D11_MAP_READ, 0, &subresource);
		contents[face].resize(desc.Width * desc.Height);
		for (UINT y = 0; y < desc.Height; ++y)
		{
			memcpy(contents[face].data() + desc.Width * y, (const char*)subresource.pData + subresource.RowPitch * y, sizeof(Vector4) * desc.Width);
		}

		context->Unmap(tex->getTexture(), index);
	}
}


void IrradianceVolumes::render(Renderer::Texture2D::Ptr rt)
{
	//mCal();
	auto quad = getQuad();
	auto renderer = getRenderer();

	quad->setBlendColorAdd();
	//quad->setDefaultBlend(false);
	quad->setDefaultViewport();
	quad->setPixelShader(mPS);
	
	auto sampler = renderer->createSampler("linear_clamp_3d", 
		D3D11_FILTER_MIN_MAG_MIP_LINEAR, 
		D3D11_TEXTURE_ADDRESS_CLAMP, 
		D3D11_TEXTURE_ADDRESS_CLAMP, 
		D3D11_TEXTURE_ADDRESS_CLAMP);

	quad->setSamplers({ sampler });

	std::vector<Renderer::ShaderResource::Ptr> srvs =
	{
		getShaderResource("albedo"),
		getShaderResource("depth"),
		getShaderResource("normal"),
		getShaderResource("material"),
	};

	for (auto& c : mCoefficients)
		srvs.push_back(c);

	quad->setTextures(srvs);
	quad->setRenderTarget(rt);

	auto root = getScene()->getRoot();
	auto aabb = root->getWorldAABB();
	auto vec = aabb.second - aabb.first;
	auto len = vec / (mSize);
	auto half = len * 0.5f;

	auto cam = getCamera();
	Constants c;
	c.intensity = getValue<float>("IVintensity");
	c.invertViewProj = (cam->getViewMatrix() * cam->getProjectionMatrix()).Invert().Transpose();
	c.range = vec - len ;
	c.origin = aabb.first + half;
	c.campos = cam->getNode()->getRealPosition();
	mConstants->blit(c);
	quad->setConstant(mConstants);
	quad->draw();

}
