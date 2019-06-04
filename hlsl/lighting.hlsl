#include "pbr.hlsl"

Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);
Buffer<float4> pointlights: register(t3);
Buffer<float4> spotlights: register(t4);
Buffer<float4> dirlights: register(t5);


#if TILED || CLUSTERED
Buffer<uint> lightsIndices: register(t6);
#if TILED
Texture2D<uint4> tiles:register(t7);
#else
Texture3D<uint4> clusters: register(t7);
#endif
#endif


SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	matrix View;
	float3 clustersize;
	int numdirs;
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
float3 travelLights(uint pointoffset, uint pointnum, uint spotoffset, uint spotnum, float3 N, float3 pos, float3 albedo)
{
	float3 F0 = F0_DEFAULT;
	float3 Lo = 0;
	for (uint i = 0; i < pointnum; ++i)
	{
		uint index = lightsIndices[pointoffset + i];

		float4 lightpos = pointlights[index * 2]; // pos and range
		float4 lightcolor = pointlights[index * 2 + 1];

		float3 L = normalize(lightpos.xyz - pos);

		float dist = length(pos - lightpos.xyz);
		float3 radiance = pointlight(dist, lightpos.w, lightcolor.rgb);
		Lo += BRDF(roughness, metallic, F0, albedo, N, L, -pos.xyz) * radiance;
	}

	for (uint i = 0; i < spotnum; ++i)
	{
		uint index = lightsIndices[spotoffset + i];
		float4 lightpos = spotlights[index * 3]; // pos and range
		float4 lightdir = spotlights[index * 3 + 1]; // dir and phi
		float4 lightcolor = spotlights[index * 3 + 2];

		float3 L = normalize(lightpos.xyz - pos);

		float dist = length(pos - lightpos.xyz);
		float3 radiance = spotlight(dist, lightpos.w, dot(-L, lightdir.xyz), lightdir.w, lightcolor.rgb);
		Lo += BRDF(roughness, metallic, F0, albedo, N, L, -pos.xyz) * radiance ;
	}

	for (uint i = 0; i < numdirs; ++i)
	{
		float4 lightdir = dirlights[i * 2];
		float4 lightcolor = dirlights[i * 2 + 1];

		float3 L = normalize(-lightdir.xyz);
		float3 radiance = lightcolor.xyz;
		Lo += BRDF(roughness, metallic, F0, albedo, N, L, -pos.xyz) * radiance;
	}
	return Lo;
}

#endif


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

	uint4 tile = tiles[uint2(x,y)];
	Lo += travelLights(tile.x, tile.y, tile.x + tile.y, tile.z, N, viewPos.xyz, albedo);

#elif CLUSTERED
	uint x = input.Pos.x / clustersize.x;
	uint y = input.Pos.y / clustersize.y;
	uint z = viewToSlice(viewPos.z);
	uint3 coord = uint3(x, y, z);

	uint4 cluster = clusters[coord];

	Lo += travelLights(cluster.x, cluster.y, cluster.x + cluster.y, cluster.z, N, viewPos.xyz, albedo);
#else


#if POINT
	float4 lightpos = pointlights[input.index * 2];
	float3 lightcolor = pointlights[input.index * 2 + 1].rgb;

	float3 L = normalize(lightpos.xyz - viewPos.xyz);
	float distance = length(lightpos.xyz - viewPos.xyz);
	float3 radiance = pointlight(distance, lightpos.w, lightcolor);
	Lo += BRDF(roughness, metallic, F0, albedo, N, L, -viewPos.xyz) * radiance;

#elif SPOT
	float4 lightpos = spotlights[input.index * 3];
	float4 lightdir = spotlights[input.index * 3 + 1];
	float3 lightcolor = spotlights[input.index * 3 + 2].rgb;

	float3 L = normalize(lightpos.xyz - viewPos.xyz);
	float distance = length(lightpos.xyz - viewPos.xyz);
	float3 radiance = spotlight(distance, lightpos.w, dot(-L, lightdir.xyz), lightdir.w, lightcolor.rgb);
	Lo += BRDF(roughness, metallic, F0, albedo, N, L, -viewPos.xyz) * radiance;

#elif DIR

#endif
	
#endif
	Lo += albedo * ambient;

	return float4(Lo, 1);
}

