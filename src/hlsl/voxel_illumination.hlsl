#include "pbr.hlsl"

Texture3D<uint> albedoTex:register(t0);
Texture3D<uint> normalTex:register(t1);
Texture3D<uint> materialTex:register(t2);

Buffer<float4> pointlights:register(t3);
Buffer<float4> dirlights:register(t4);

RWTexture3D<float3> targetTex:register(u0);

cbuffer Constant:register(c0)
{
	int size;
	int numpoints;
	int numdirs;
};

SamplerState sampLinear: register(s0);

#ifndef SLICE 
#define SLICE 1
#endif

groupshared float3 albedo;
groupshared float3 normal;
groupshared float roughness;
groupshared float metallic;

float4 uintTofloat4(uint val)
{
	return float4(float((val & 0x000000FF)),
		float((val & 0x0000FF00) >> 8U),
		float((val & 0x00FF0000) >> 16U),
		float((val & 0xFF000000) >> 24U)) / 255;
}

bool checkVoxelExist(uint val)
{
	return (val & 0xFF000000) != 0;
}


bool rayMarch(float3 pos, float3 dir)
{
	float3 inv = 1 / dir;
	float step = min(min(abs(inv.x), abs(inv.y)), abs(inv.z));
	
	float maxstep = size;
	float numstep = 1;

	while (numstep < maxstep)
	{
		float3 hitpoint = pos + dir * numstep * step;
		if (checkVoxelExist(albedoTex.Sample(sampLinear, hitpoint)))
			return true;
	}
	return false;
}

float testVisibility(float3 pos, float3 dir)
{
	if (rayMarch(pos, dir))
		return 0;
	return 1;
}


[numthreads(SLICE, 1, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	uint threadindex = localIdx.x + localIdx.y;
	if (threadindex == 0)
	{
		albedo = uintTofloat4(albedoTex.Sample(sampLinear, globalIdx)).rgb;
		normal = uintTofloat4(normalTex.Sample(sampLinear, globalIdx)).rgb;
		normal = normal * 2 - 1;
		float3 material = uintTofloat4(materialTex.Sample(sampLinear, globalIdx)).rgb;
		roughness = material.r;
		metallic = material.g;
	}

	GroupMemoryBarrierWithGroupSync();



}