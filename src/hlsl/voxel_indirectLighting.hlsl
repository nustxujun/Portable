#include "voxel_cone_tracing.hlsl"
#include "pbr.hlsl"

Texture3D<float4> voxelTex:register(t0);
Texture2D depthTex:register(t1);
Texture2D normalTex:register(t2);
Texture2D albedoTex:register(t3);


SamplerState samp: register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


cbuffer Constants:register(c0)
{
	matrix invertViewProj;
	float3 offset;
	float scaling;

	float3 campos;
	int numMips;

	float range;
}


float3 getPosInVoxelSpace(float3 wp)
{
	return (wp - offset) / scaling;
}


float3x3 calTNB(float3 normal)
{
	float3 up = dot(normal, float3(0, 1, 0)) > 0.99999 ? float3(0, 0, 1) : float3(0, 1, 0);
	float3 tangent = cross( up, normal);
	float3 bitangent = cross(tangent, normal);

	return float3x3(tangent, normal,bitangent);
}

const static float3 diffuseConeDirs[] =
{
	float3(0.0f, 1.0f, 0.0f),
	float3(0.0f, 0.5f, 0.866025f),
	float3(0.823639, 0.5, 0.267617),
	float3(0.509037, 0.5, -0.700629),
	float3(-0.509037, 0.5, -0.700629),
	float3(-0.823639, 0.5,0.267617),

	//float3(0.0f, 1.0f, 0.0f),
	//float3(0.0f, 0.5f, 0.866025f),
	//float3(0.754996f, 0.5f, -0.4330128f),
	//float3(-0.754996f, 0.5f, -0.4330128f)
};

const static float diffuseConeWeights[] =
{

	PI / 4.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,

	//PI / 3.0f,
	//2.0f * PI / 9.0f,
	//2.0f * PI / 9.0f,
	//2.0f * PI / 9.0f,


};


float3 calDiffuse(float3 N, float3 worldpos)
{
	// here we make normal as Y-axis
	float3x3 TNB = calTNB(N);

	ConeTracingParams ctr;
	ctr.start = getPosInVoxelSpace(worldpos.xyz);
	//ctr.dir = R;
	ctr.octree = voxelTex;
	ctr.samp = samp;
	ctr.numMips = numMips;
	ctr.theta = 1.04;
	//ctr.theta = 0.785;

	ctr.range = range;

	float4 color = 0;
	for (int i = 0; i < 4; ++i)
	{
		ctr.dir = mul(diffuseConeDirs[i], TNB);
		color += coneTracing(ctr) * diffuseConeWeights[i];
	}
	return color.rgb * color.a ;
}

float3 calSpecular(float3 N, float3 R, float3 worldpos)
{
	ConeTracingParams ctr;
	ctr.start = getPosInVoxelSpace(worldpos.xyz);
	ctr.octree = voxelTex;
	ctr.samp = samp;
	ctr.numMips = numMips;
	ctr.range = range;
	ctr.dir = R;
	ctr.theta = 0.2;
	float4 color = coneTracing(ctr);
	return color.rgb * color.a;
}

float4 main(PS_INPUT input):SV_TARGET
{
	float2 uv = input.Tex;
	float3 albedo = albedoTex.SampleLevel(samp, uv, 0).rgb;
	float4 normalData = normalTex.SampleLevel(samp, uv, 0);
	float3 N = normalize(normalData.xyz);

	float depthVal = depthTex.SampleLevel(samp, uv, 0).r;
	float4 worldpos;
	worldpos.x = uv.x * 2.0f - 1.0f;
	worldpos.y = -(uv.y * 2.0f - 1.0f);
	worldpos.z = depthVal;
	worldpos.w = 1.0f;
	worldpos = mul(worldpos, invertViewProj);
	worldpos /= worldpos.w;

	float3 V = normalize(campos - worldpos.xyz);
	float3 R = normalize(reflect(-V, N));



	float3 color = calDiffuse(N, worldpos.xyz) * albedo;
	//color += calSpecular(N, R, worldpos.xyz);
	 
	return float4(color, 1);
	//return voxelTex.SampleLevel(samp, ctr.start / range, 0);
}