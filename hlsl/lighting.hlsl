#include "pbr.hlsl"

Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);
TextureCube irradianceTexture: register(t3);


Buffer<float4> pointlights: register(t6);
Buffer<float4> spotlights: register(t7);
Buffer<float4> dirlights: register(t8);


#if TILED || CLUSTERED
Buffer<uint> lightsIndices: register(t9);
#if TILED
Texture2D<uint4> tiles:register(t10);
#else
Texture3D<uint4> clusters: register(t10);
#endif
#endif


#ifndef NUM_SHADOWMAPS
#define NUM_SHADOWMAPS 8
#endif
Texture2D shadows[NUM_SHADOWMAPS]:register(t20);


SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	float3 clustersize;
	uint numdirs;
	float3 campos;
	float roughness;
	float metallic;
	float width;
	float height;
	float near;
	float far;
	float ambient;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
#if !(TILED || CLUSTERED)
	uint index:TEXCOORD0;
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
#if TILED || CLUSTERED
	if (dist > range)
		return 0;
#endif
	return color * attenuate(dist, range)  ;
}

float3 spotlight(float dist, float range,  float theta, float phi, float3 color)
{
#if TILED || CLUSTERED
	if (dist > range)
		return 0;
#endif
	if (theta < phi)
		return 0;

	return color * attenuate(dist, range);
}


#if TILED || CLUSTERED
float3 travelLights(uint pointoffset, uint pointnum, uint spotoffset, uint spotnum, float3 N, float3 pos, float3 albedo, float2 uv)
{
	float3 F0 = F0_DEFAULT;
	float3 Lo = 0;
	float3 V = normalize(campos.xyz - pos);

	float shadowlist[NUM_SHADOWMAPS];
	shadowlist[0] = 1.0f;
	for (uint i = 1; i < NUM_SHADOWMAPS; ++i)
	{
		shadowlist[i ] = shadows[i - 1].SampleLevel(sampPoint, uv, 0).r;
	}

	for (uint i = 0; i < pointnum; ++i)
	{
		uint index = lightsIndices[pointoffset + i];

		float4 lightpos = pointlights[index * 2]; // pos and range
		float4 lightcolor = pointlights[index * 2 + 1]; // color and shadow index

		uint shadowindex = lightcolor.w;
		float shadow = shadowlist[shadowindex];

		float3 L = normalize(lightpos.xyz - pos);

		float dist = length(pos - lightpos.xyz);
		float3 radiance = pointlight(dist, lightpos.w, lightcolor.rgb) ;
		Lo += directBRDF(roughness, metallic, F0, albedo, N, L, -pos.xyz) * radiance * shadow;
	}

	for (uint i = 0; i < spotnum; ++i)
	{
		uint index = lightsIndices[spotoffset + i];
		float4 lightpos = spotlights[index * 3]; // pos and range
		float4 lightdir = spotlights[index * 3 + 1]; // dir and phi
		float4 lightcolor = spotlights[index * 3 + 2];

		uint shadowindex = lightcolor.w;
		float shadow = shadowlist[shadowindex];

		float3 L = normalize(lightpos.xyz - pos);

		float dist = length(pos - lightpos.xyz);
		float3 radiance = spotlight(dist, lightpos.w, dot(-L, lightdir.xyz), lightdir.w, lightcolor.rgb) ;
		Lo += directBRDF(roughness, metallic, F0, albedo, N, L, V) * radiance * shadow;
	}

	for (uint i = 0; i < numdirs; ++i)
	{
		float4 lightdir = dirlights[i * 2];
		float4 lightcolor = dirlights[i * 2 + 1];

		uint shadowindex = lightcolor.w;
		float shadow = shadowlist[shadowindex];

		float3 L = normalize(-lightdir.xyz);
		float3 radiance = lightcolor.xyz;
		Lo += directBRDF(roughness, metallic, F0, albedo, N, L, V) * radiance * shadow;
	}

	float3 irradiance = irradianceTexture.Sample(sampLinear, N).rgb;
	Lo += indirectBRDF(irradiance, albedo);
	return Lo;
}

#endif


float4 main(PS_INPUT input) : SV_TARGET
{
	float2 texcoord = float2(input.Pos.x / width, input.Pos.y / height);
	float3 albedo = albedoTexture.Sample(sampLinear, texcoord).rgb;

	float4 normalData = normalTexture.Sample(sampPoint, texcoord);
	float3 N = normalize(normalData.xyz);
	float depthVal = depthTexture.Sample(sampPoint, texcoord).g;
	float4 worldpos;
	worldpos.x = texcoord.x * 2.0f - 1.0f;
	worldpos.y = -(texcoord.y * 2.0f - 1.0f);
	worldpos.z = depthVal;
	worldpos.w = 1.0f;
	worldpos = mul(worldpos, invertViewProj);
	worldpos /= worldpos.w;


	float3 F0 = F0_DEFAULT;

	float3 Lo = 0;


#if TILED

	uint x = input.Pos.x;
	uint y = input.Pos.y;
	x /= 16;
	y /= 16;

	uint4 tile = tiles[uint2(x,y)];
	Lo += travelLights(tile.x, tile.y, tile.x + tile.y, tile.z, N, worldpos.xyz, albedo, texcoord);

#elif CLUSTERED
	uint x = input.Pos.x / clustersize.x;
	uint y = input.Pos.y / clustersize.y;
	uint z = viewToSlice(worldpos.z);
	uint3 coord = uint3(x, y, z);

	uint4 cluster = clusters[coord];

	Lo += travelLights(cluster.x, cluster.y, cluster.x + cluster.y, cluster.z, N, worldpos.xyz, albedo, texcoord);
#else


#if POINT
	float4 lightpos = pointlights[input.index * 2];
	float3 lightcolor = pointlights[input.index * 2 + 1].rgb;

	float3 L = normalize(lightpos.xyz - worldpos.xyz);
	float distance = length(lightpos.xyz - worldpos.xyz);
	float3 radiance = pointlight(distance, lightpos.w, lightcolor);
	Lo += directBRDF(roughness, metallic, F0, albedo, N, L, -worldpos.xyz) * radiance;

#elif SPOT
	float4 lightpos = spotlights[input.index * 3];
	float4 lightdir = spotlights[input.index * 3 + 1];
	float3 lightcolor = spotlights[input.index * 3 + 2].rgb;

	float3 L = normalize(lightpos.xyz - worldpos.xyz);
	float distance = length(lightpos.xyz - worldpos.xyz);
	float3 radiance = spotlight(distance, lightpos.w, dot(-L, lightdir.xyz), lightdir.w, lightcolor.rgb);
	Lo += directBRDF(roughness, metallic, F0, albedo, N, L, -worldpos.xyz) * radiance;

#elif DIR

#endif
	
#endif
	Lo += albedo * ambient;


	return float4(Lo, 1);
}

