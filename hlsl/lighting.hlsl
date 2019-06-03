#include "pbr.hlsl"

Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);
Buffer<float4> lights: register(t3);

#ifdef TILED
Buffer<uint> lightsIndex: register(t4);
#endif

#ifdef CLUSTERED
Buffer<uint> lightsIndices: register(t4);
Texture3D<uint2> clusters: register(t5);


#endif

SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	matrix View;
	float3 clustersize;
	float roughness;
	float metallic;
	float width;
	float height;
	int maxLightsPerTile;
	int tilePerline;
	float near;
	float far;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
#ifndef TILED
#ifndef CLUSTERED
	uint index:TEXCOORD0;
#endif
#endif
};



#define NUM_SLICES 16

uint viewToSlice(float z)
{
	return (uint)floor(NUM_SLICES * log(z / near) / log(far / near));
	//return NUM_SLICES * (z - near) / (far - near);
}


float3 pointlight(float dist, float range, float3 color)
{
#ifdef TILED || CLUSTERED
	if (dist > range)
		return 0;
#endif
	return color * attenuate(dist, range) / (dist * dist);
}





float4 main(PS_INPUT input) : SV_TARGET
{
	float2 texcoord = float2(input.Pos.x / width, input.Pos.y / height);
	float4 texcolor = albedoTexture.Sample(sampLinear, texcoord);
	float3 albedo = pow(texcolor.rgb, 2.2);

	float4 normalData = normalTexture.Sample(sampPoint, texcoord);
	normalData = mul(normalData, View);
	float3 N = normalize(normalData.xyz);
	float depthVal = depthTexture.Sample(sampPoint, texcoord).g;
	float4 viewPos;
	viewPos.x = texcoord.x * 2.0f - 1.0f;
	viewPos.y = -(texcoord.y * 2.0f - 1.0f);
	viewPos.z = depthVal;
	viewPos.w = 1.0f;
	viewPos = mul(viewPos, invertProj);
	viewPos /= viewPos.w;


	float3 F0 = F0_DEFAULT;

	float3 Lo = 0;


#if TILED

	int x = input.Pos.x;
	int y = input.Pos.y;
	x /= 16;
	y /= 16;

	int startoffset = (x + y * tilePerline) * (maxLightsPerTile + 1);
	uint num = lightsIndex[startoffset];
	for (uint i = 1; i <= num; ++i)
	{
		uint index = lightsIndex[startoffset + i];
		float4 lightpos = lights[index * 2];
		float3 lightcolor = lights[index * 2 + 1].rgb;

#elif CLUSTERED
	uint x = input.Pos.x / clustersize.x;
	uint y = input.Pos.y / clustersize.y;
	uint z = viewToSlice(viewPos.z);
	uint3 coord = uint3(x, y, z);

	uint2 cluster = clusters[coord];

	//float4 final = float4((float)x / 256.0f, (float)y / 256.0f, (float)z/ 16.0f,1);
	//float4 final = (float)cluster.y / (float)100;
	//return (float)cluster.y / (float)100;
	//return (float)cluster.x / 16.0f * 100.0f;
	for (uint i = 0; i < cluster.y; ++i)
	{
		uint index = lightsIndices[cluster.x + i];
		float4 lightpos = lights[index * 2];
		float3 lightcolor = lights[index * 2 + 1].rgb;

#else

	float4 lightpos = lights[input.index * 2];
	float3 lightcolor = lights[input.index * 2 + 1].rgb;
#endif

#ifdef POINT
	float3 L = normalize(lightpos.xyz - viewPos.xyz);


	float distance = length(lightpos.xyz - viewPos.xyz);
	float3 radiance = pointlight(distance, lightpos.w, lightcolor);
	

#endif
#ifdef DIR
	float3 L = -normalize(lightpos.xyz);
	float3 radiance = lightcolor;

#endif
#ifdef SPOT
	float3 L = -normalize(lightpos.xyz - viewPos);
	float3 radiance = lightcolor;

#endif


	Lo += BRDF(roughness, metallic, F0, albedo, N, L, -viewPos.xyz) * radiance;
#if  TILED 
	}
#endif
#if CLUSTERED
	}

	
#endif


	//float3 ambient = 0.01 * albedo;// *ao;
	//float3 color = ambient + Lo;


	float3 color = Lo;
	//color = color / (color + 1);
	//color = pow(color, 1.0 / 2.2);

	return float4(color, texcolor.a);
}

