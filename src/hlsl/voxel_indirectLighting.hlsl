#include "voxel_cone_tracing.hlsl"

Texture3D<float4> voxelTex:register(t0);
Texture2D depthTex:register(t1);
Texture2D normalTex:register(t2);


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
}


float3 getPosInVoxelSpace(float3 wp)
{
	return (wp - offset) / scaling;
}


float4 main(PS_INPUT input):SV_TARGET
{
	float2 uv = input.Tex;
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

	ConeTracingParams ctr;
	ctr.start = getPosInVoxelSpace(worldpos.xyz);
	ctr.dir = R;
	ctr.octree = voxelTex;
	ctr.samp = samp;
	ctr.numMips = numMips;
	ctr.theta = 0.52;
	float4 color = coneTracing(ctr);

	return voxelTex.Load(int4(ctr.start,0));
	//return float4(worldpos.xyz,1);
}