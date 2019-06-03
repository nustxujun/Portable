#include "math.hlsl"
Texture2D depthboundsTex: register(t0);
Buffer<float4> lights: register(t1);
RWBuffer<uint> lightsOutput: register(u0);


cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	int numLights;
	float texelwidth;
	float texelheight;
	int maxLightsPerTile;
	int tilePerline;
}



[numthreads(1, 1,1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx: SV_GroupID)
{
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

	uint startOffset = (globalIdx.x + globalIdx.y * tilePerline) * (maxLightsPerTile + 1);
	int lightcount = 0;
	for (int i = 0; i < numLights; ++i)
	{
		float4 light = lights[i * 2];
		float3 pos = light.xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbcenter, aabbhalf) )
			continue;

		lightsOutput[startOffset + 1 + lightcount] = i;

		lightcount++;
		if (lightcount >= maxLightsPerTile)
			break;
	}
	lightsOutput[startOffset] = lightcount;
}