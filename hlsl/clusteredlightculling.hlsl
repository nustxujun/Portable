#include "math.hlsl"
Buffer<float4> pointlights:register(t0);
Buffer<float4> spotlights:register(t1);

RWBuffer<uint> curIndex:register(u0);
RWBuffer<uint> lightIndices:register(u1);
RWTexture3D<uint4> clusters:register(u2);


cbuffer Constants: register(c0)
{
	matrix invertProj;
	matrix view;
	float3 slicesSize;
	int numPointLights;
	int numSpotLights;
	float nearZ;
	float farZ;
};


#ifndef LIGHT_THREAD
#define LIGHT_THREAD 1
#endif

#ifndef MAX_LIGHTS_PER_CLUSTER
#define MAX_LIGHTS_PER_CLUSTER 1024
#endif
float sliceZ(uint level)
{
	float k = (float)level * (1.0f / (float)slicesSize.z);
	return nearZ * pow(farZ / nearZ, k);
	//return nearZ + (farZ - nearZ) * k;
}


void genAABB(in float3 frontlt, in float3 frontrb, in float3 backlt, in float3 backrb, out float3 center, out float3 halflen)
{
	float3 aabbmin = float3(min(backlt.x, frontlt.x), min(backrb.y, frontrb.y), frontlt.z);
	float3 aabbmax = float3(max(backrb.x, frontrb.x), max(backlt.y, frontlt.y), backlt.z);

	center = (aabbmax + aabbmin) * 0.5f;
	halflen = (aabbmax - aabbmin) * 0.5f;
}

void genLTRB(in float radio, in float4 flt, in float4 frb, in float z, out float3 lt, out float3 rb)
{
	lt = float3(radio * flt.x, radio * flt.y, z);
	rb = float3(radio * frb.x, radio * frb.y, z);
}

groupshared float3 aabbCenter;
groupshared float3 aabbHalf;
groupshared uint groupPointCount;
groupshared uint groupSpotCount;
groupshared uint groupPointlights[MAX_LIGHTS_PER_CLUSTER];
groupshared uint groupSpotlights[MAX_LIGHTS_PER_CLUSTER];

[numthreads(LIGHT_THREAD, 1, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	uint threadindex = localIdx.x + localIdx.y;
	if (threadindex == 0)
	{
		groupPointCount = 0;
		groupSpotCount = 0;
		float texelwidth = 1.0f / slicesSize.x;
		float texelheight = 1.0f / slicesSize.y;

		float2 coord = float2(float(groupIdx.x) * texelwidth, float(groupIdx.y) * texelheight);
		float2 lt = float2((float)coord.x  * 2.0f - 1.0f, 1.0f - (float)coord.y * 2.0f);
		float2 rb = float2((float)(coord.x + texelwidth) * 2.0f - 1.0f, 1.0f - (float)(coord.y + texelheight)* 2.0f);

		float4 flt = mul(float4(lt, 1, 1), invertProj);
		flt /= flt.w;
		float4 frb = mul(float4(rb, 1, 1), invertProj);
		frb /= frb.w;

		float near = sliceZ(groupIdx.z);
		float far = sliceZ(groupIdx.z + 1);

		float4 back = float4(flt.xy, frb.xy) * (far / farZ);
		float4 front = float4(flt.xy, frb.xy) * (near / farZ);

		createAABBFromFrustum(front, back , near, far, aabbCenter, aabbHalf);
	}

	GroupMemoryBarrierWithGroupSync();

	for (int i = 0; i < numPointLights; i += LIGHT_THREAD)
	{
		float4 light = pointlights[i * 2];
		float3 pos = mul(float4(light.xyz,1.0), view).xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbCenter, aabbHalf))
			continue;

		uint index;
		InterlockedAdd(groupPointCount, 1, index);
		if (index < MAX_LIGHTS_PER_CLUSTER)
			groupPointlights[index] = i;
		else
			break;
	}

	for (int i = 0; i < numSpotLights; i += LIGHT_THREAD)
	{
		float4 light = spotlights[i * 3];
		float3 pos = mul(float4(light.xyz, 1.0), view).xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbCenter, aabbHalf))
			continue;

		uint index;
		InterlockedAdd(groupSpotCount, 1, index);
		if (index < MAX_LIGHTS_PER_CLUSTER)
			groupSpotlights[index] = i;
		else
			break;
	}

	if (threadindex != 0)
		return;

	groupPointCount = min(groupPointCount, MAX_LIGHTS_PER_CLUSTER);
	groupSpotCount = min(groupSpotCount, MAX_LIGHTS_PER_CLUSTER);

	uint offset;
	InterlockedAdd(curIndex[0], (groupPointCount + groupSpotCount), offset);
	clusters[groupIdx] = uint4(offset, groupPointCount, groupSpotCount, 0);
	for (uint i = 0; i < groupPointCount; ++i)
		lightIndices[offset + i] = groupPointlights[i];

	for (uint i = 0; i < groupSpotCount; ++i)
		lightIndices[offset + i + groupPointCount] = groupSpotlights[i];
}

