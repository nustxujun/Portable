#include "pbr.hlsl"

Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);
Texture2D materialTexture:register(t3);
TextureCube irradianceTexture: register(t4);
TextureCube prefilteredTexture: register(t5);
Texture2D lutTexture: register(t6);


Buffer<float4> pointlights: register(t7);
Buffer<float4> spotlights: register(t8);
Buffer<float4> dirlights: register(t9);

#define PREFILTERED_MIP_LEVEL 5

#if TILED || CLUSTERED
Buffer<uint> lightsIndices: register(t10);
#if TILED
Texture2D<uint4> tiles:register(t11);
#else
Texture3D<uint4> clusters: register(t11);
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
	matrix view;
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
#if !(TILED || CLUSTERED || DIR)
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


#if TILED || CLUSTERED || DIR
float3 travelLights(float roughness, float metallic,uint pointoffset, uint pointnum, uint spotoffset, uint spotnum, float3 N, float3 pos, float3 albedo, float2 uv)
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

#if (TILED || CLUSTERED)
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
		Lo += directBRDF(roughness, metallic, F0, albedo, N, L, V) * radiance * shadow;
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
#endif

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

	float3 reflectcoord = normalize(reflect(-V, N));

	float3 lut = LUT(N, V, roughness, lutTexture, sampPoint);
	float3 prefiltered = prefilteredTexture.SampleLevel(sampPoint, reflectcoord, roughness * (PREFILTERED_MIP_LEVEL -1));
	float3 irradiance = irradianceTexture.SampleLevel(sampPoint, N, 0).rgb;
	Lo += indirectBRDF(irradiance, prefiltered, lut, roughness, metallic, F0, albedo, N, V);
	return Lo;
}

#endif


float4 main(PS_INPUT input) : SV_TARGET
{
	float2 texcoord = float2(input.Pos.x / width, input.Pos.y / height);
	float3 albedo = albedoTexture.SampleLevel(sampPoint, texcoord, 0).rgb;
	float2 material = materialTexture.SampleLevel(sampPoint, texcoord, 0).rg;

	float4 normalData = normalTexture.SampleLevel(sampPoint, texcoord, 0);
	float3 N = normalize(normalData.xyz);
	float depthVal = depthTexture.SampleLevel(sampPoint, texcoord, 0).g;
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
	Lo += travelLights(roughness* material.x, metallic * material.y, tile.x, tile.y, tile.x + tile.y, tile.z, N, worldpos.xyz, albedo, texcoord);

#elif CLUSTERED

	float4 viewpos = mul(worldpos, view);
	uint x = input.Pos.x / clustersize.x;
	uint y = input.Pos.y / clustersize.y;
	uint z = viewToSlice(viewpos.z);
	uint3 coord = uint3(x, y, z);

	uint4 cluster = clusters[coord];
	Lo += travelLights(roughness * material.x , metallic * material.y, cluster.x, cluster.y, cluster.x + cluster.y, cluster.z, N, worldpos.xyz, albedo, texcoord);
#else


	float3 V = normalize(campos - worldpos.xyz);
#if POINT
	float4 lightpos = pointlights[input.index * 2];
	float3 lightcolor = pointlights[input.index * 2 + 1].rgb;

	float3 L = normalize(lightpos.xyz - worldpos.xyz);
	float distance = length(lightpos.xyz - worldpos.xyz);

	float3 radiance = pointlight(distance, lightpos.w, lightcolor);
	Lo += directBRDF(roughness * material.x, metallic * material.y, F0, albedo, N, L, V) * radiance;

#elif SPOT
	float4 lightpos = spotlights[input.index * 3];
	float4 lightdir = spotlights[input.index * 3 + 1];
	float3 lightcolor = spotlights[input.index * 3 + 2].rgb;

	float3 L = normalize(lightpos.xyz - worldpos.xyz);
	float distance = length(lightpos.xyz - worldpos.xyz);

	float3 radiance = spotlight(distance, lightpos.w, dot(-L, lightdir.xyz), lightdir.w, lightcolor.rgb);
	Lo += directBRDF(roughness * material.x, metallic * material.y ,F0, albedo, N, L, V) * radiance;

#elif DIR
	Lo += travelLights(roughness * material.x, metallic * material.y, 0, 0, 0, 0, N, worldpos.xyz, albedo, texcoord);
#endif
	
#endif
	Lo += albedo * ambient;


	return float4(Lo, 1);
}

