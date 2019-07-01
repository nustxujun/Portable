#include "pbr.hlsl"

Texture2D albedoTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D materialTex:register(t2);
Texture2D depthTex:register(t3);
#if IBL
#define PREFILTERED_MIP_LEVEL 5

TextureCube irradianceTexture: register(t4);
TextureCube prefilteredTexture: register(t5);
Texture2D lutTexture: register(t6);
#else
TextureCube envTex: register(t4);
#endif


cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	float3 campos;
	float envCubeScale;
	float3 envCubeSize;
	float intensity;
	float3 envCubePos;
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


float3 intersectBox(float3 pos, float3 reflectionVector, float3 cubeSize, float3 cubePos)
{
	float3 FirstPlaneIntersect = (cubePos + cubeSize * 0.5f - pos) / reflectionVector;
	float3 SecondPlaneIntersect = (cubePos - cubeSize * 0.5f - pos) / reflectionVector;
	// Get the furthest of these intersections along the ray
	// (Ok because x/0 give +inf and -x/0 give ¨Cinf )
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	// Find the closest far intersection
	float Distance = min(min(FurthestPlane.x, FurthestPlane.y), FurthestPlane.z);

	// Get the intersection position
	return pos + reflectionVector * Distance;



	//float3 rbmax = (0.5f * (cubeSize - cubePos) - pos) / reflectionVector;
	//float3 rbmin = (-0.5f * (cubeSize - cubePos) - pos) / reflectionVector;

	//float3 rbminmax = float3(
	//	(reflectionVector.x > 0.0f) ? rbmax.x : rbmin.x,
	//	(reflectionVector.y > 0.0f) ? rbmax.y : rbmin.y,
	//	(reflectionVector.z > 0.0f) ? rbmax.z : rbmin.z);

	//float correction = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
	//return (pos + reflectionVector * correction);
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

#if CORRECTED
	float3 corrected = intersectBox(worldpos.xyz, R, envCubeSize * envCubeScale, envCubePos);
	R = normalize(corrected - envCubePos);
#endif


#if IBL
	float3 lut = LUT(N, V, roughness, lutTexture, sampLinear);
	float3 prefiltered = prefilteredTexture.SampleLevel(sampLinear, R, roughness * (PREFILTERED_MIP_LEVEL -1)).rgb;
	float3 irradiance = irradianceTexture.SampleLevel(sampLinear, N, 0).rgb;
	result = indirectBRDF(irradiance, prefiltered, lut, roughness, metallic, F0_DEFAULT, albedo, N, V);
	return float4(result, 1);
#else
	result = envTex.SampleLevel(sampLinear, R, 0).rgb;
	return float4(result * intensity * (1 - roughness) * metallic * reflection , 1);
#endif
}


