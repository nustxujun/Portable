#include "math.hlsl"
Texture2D depthboundsTex: register(t0);
Buffer<float4> pointlights: register(t1);
Buffer<float4> spotlights:register(t2);

RWBuffer<uint> curIndex:register(u0);
RWBuffer<uint> lightIndices:register(u1);
RWTexture2D<uint4> tiles: register(u2);


cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	int numPointLights;
	int numSpotLights;
	float texelwidth;
	float texelheight;
}

#ifndef MAX_LIGHTS_PER_TILE
#define MAX_LIGHTS_PER_TILE 1024
#endif

[numthreads(1, 1,1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx: SV_GroupID)
{
	uint indices[MAX_LIGHTS_PER_TILE * 2];

	float2 db = depthboundsTex.Load(uint3(globalIdx.x, globalIdx.y, 0)).rg;
	float depthmin = db.x;
	float depthmax = db.y;

	float2 coord = float2(float(globalIdx.x) * texelwidth, float(globalIdx.y) * texelheight);

	float2 lt = float2((float)coord.x  * 2.0f - 1.0f, 1.0f - (float)coord.y * 2.0f);
	float2 rb = float2((float)(coord.x + 0.0625f) * 2.0f - 1.0f, 1.0f - (float)(coord.y + 0.0625f)* 2.0f);

	float4 flt = mul(float4(lt, 1, 1), invertProj);
	flt /= flt.w;
	float4 frb = mul(float4(rb, 1, 1), invertProj);
	frb /= frb.w;


	float4 back = float4(flt.xy, frb.xy) * (depthmax / flt.z);
	float4 front = float4(flt.xy, frb.xy) * (depthmin / flt.z);

	float3 aabbcenter, aabbhalf;
	createAABBFromFrustum(front, back, depthmin, depthmax, aabbcenter, aabbhalf);

	int pointcount = 0;
	for (int i = 0; i < numPointLights; ++i)
	{
		float4 light = pointlights[i * 2];
		float3 pos = light.xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbcenter, aabbhalf) )
			continue;

		indices[pointcount++] = i;

		if (pointcount >= MAX_LIGHTS_PER_TILE)
			break;
	}

	int spotcount = 0;
	for (int i = 0; i < numSpotLights; ++i)
	{
		float4 light = spotlights[i * 3];
		float3 pos = light.xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbcenter, aabbhalf))
			continue;

		indices[pointcount + spotcount++] = i;

		if (spotcount >= MAX_LIGHTS_PER_TILE)
			break;
	}


	uint offset;
	InterlockedAdd(curIndex[0], (pointcount + spotcount), offset);
	tiles[groupIdx.xy] = uint4(offset, pointcount, spotcount, 0);
	for (uint i = 0; i < (pointcount + spotcount); ++i)
		lightIndices[offset + i] = indices[i];

}