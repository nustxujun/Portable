#include "pbr.hlsl"

Texture2D albedoTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D materialTex:register(t2);
Texture2D depthTex:register(t3);
Buffer<float3> probes:register(t4);

#define MAX_NUM_PROBES 10

#if IBL
#define PREFILTERED_MIP_LEVEL 5
Texture2D lutTexture: register(t5);
TextureCube irradianceTexture[MAX_NUM_PROBES]: register(t10);
TextureCube prefilteredTexture[MAX_NUM_PROBES]: register(t20);
#else
TextureCube envTex[MAX_NUM_PROBES]: register(t10);
#endif


cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	float3 campos;
	float intensity;
	int numprobes;
}

SamplerState sampLinear: register(s0);


cbuffer ConstantBuffer: register(b0)
{
	float threshold;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

bool testBox(float3 pos, float3 cubemin, float3 cubemax)
{
	return !(pos.x < cubemin.x || pos.x > cubemax.x ||
		pos.y < cubemin.y || pos.y > cubemax.y ||
		pos.z < cubemin.z || pos.z > cubemax.z);
}

float3 intersectBox(float3 pos, float3 reflectionVector, float3 cubemin, float3 cubemax)
{
	float3 FirstPlaneIntersect = (cubemax - pos) / reflectionVector;
	float3 SecondPlaneIntersect = (cubemin - pos) / reflectionVector;
	// Get the furthest of these intersections along the ray
	// (Ok because x/0 give +inf and -x/0 give ¨Cinf )
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	// Find the closest far intersection
	float Distance = min(min(FurthestPlane.x, FurthestPlane.y), FurthestPlane.z);

	// Get the intersection position
	return pos + reflectionVector * Distance;
}


float4 main(PS_INPUT Input) : SV_TARGET
{
	float2 uv = Input.Tex;
	float3 albedo = albedoTex.SampleLevel(sampLinear, uv, 0).rgb;
	float3 N = normalize(normalTex.SampleLevel(sampLinear, uv, 0).rgb);
	float4 material = materialTex.SampleLevel(sampLinear, uv, 0);
	float roughness = material.r;
	float metallic = material.g; 
	float reflection = material.b;
	float depth = depthTex.SampleLevel(sampLinear, uv, 0).r;
	float4 worldpos;
	worldpos.x = uv.x * 2.0f - 1.0f;
	worldpos.y = -(uv.y * 2.0f - 1.0f);
	worldpos.z = depth;
	worldpos.w = 1.0f;

	worldpos = mul(worldpos, invertViewProj);
	worldpos /= worldpos.w;

	float3 V = normalize(campos - worldpos.xyz);
	float3 R = normalize(reflect(-V, N));

	float3 result  = 0;

	int maxnums = min(MAX_NUM_PROBES, numprobes);
	for (int i = 0; i < maxnums; ++i)
	{
		float3 coord = R;
		float3 probepos = probes[i * 6].xyz;
		float3 boxmin = probes[i * 6 + 1].xyz;
		float3 boxmax = probes[i * 6 + 2].xyz;
		float3 infmin = probes[i * 6 + 3].xyz;
		float3 infmax = probes[i * 6 + 4].xyz;
		float3 probeintensity = probes[i * 6 + 5].xyz;
		
		if (testBox(worldpos.xyz, infmin, infmax))
		{
			float3 calcolor = 0;
#if CORRECTED
			float3 corrected = intersectBox(worldpos.xyz, R, boxmin, boxmax);
			coord = normalize(corrected - probepos);
#endif


#if IBL
			float3 lut = LUT(N, V, roughness, lutTexture, sampLinear);
			float3 prefiltered = prefilteredTexture[i].SampleLevel(sampLinear, coord, roughness * (PREFILTERED_MIP_LEVEL - 1)).rgb;
			float3 irradiance = irradianceTexture[i].SampleLevel(sampLinear, N, 0).rgb;
			calcolor = indirectBRDF(irradiance, prefiltered, lut, roughness, metallic, F0_DEFAULT, albedo, N, V);
			result += calcolor * intensity * probeintensity/** float3((i % 2), 0, (1.0f - i % 2)) */;
#else
			calcolor = envTex.SampleLevel(sampLinear, coord, 0).rgb;
			result += calcolor * intensity * (1 - roughness) * metallic * reflection, 1) ;
#endif
		}
	}



	return float4(result, 1);
}


