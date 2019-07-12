#include "pbr.hlsl"


Texture2D albedoTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D materialTex:register(t2);
Texture2D depthTex:register(t3);
TextureCube envdepthTex:register(t4);


#if IBL
#define PREFILTERED_MIP_LEVEL 5
Texture2D lutTexture: register(t5);
TextureCube irradianceTexture: register(t6);
TextureCube prefilteredTexture: register(t7);
#elif SH
#include "spherical_harmonics.hlsl"
Buffer<float3> coefs: register(t5);
#else
TextureCube envTex: register(t5);
#endif


cbuffer ConstantBuffer: register(b0)
{
	matrix proj;
	matrix invertViewProj;

	float3 campos;
	float nearZ;
	
	float3 probepos;
	float maxsteps;
	
	float3 intensity;
	float stepsize;
	
	float3 boxmin;
	float3 boxmax;
	float3 infmin;
	float3 infmax;
	
	float2 screenSize;
}

SamplerState sampLinear: register(s0);
SamplerState pointClamp : register(s1);


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


float2 toScreen(float4 pos)
{
	return float2((pos.x *0.5 + 0.5) * screenSize.x, (0.5 - pos.y * 0.5) * screenSize.y);
}

struct RayParams
{
	float3 origin;
	float3 dir;
	float jitter;
};

static const float MAX_STEPS = 10000;

bool raymarch(RayParams params,  out float3 hitpoint)
{
	float raylen = maxsteps * stepsize;

	float steplen = stepsize;

	float3 dir = params.dir;
	float3 origin = params.origin;
	float jitter = params.jitter;

	float3 endpoint = dir * raylen + origin;


	float numsteps = 0;
	hitpoint = 0;
	float stride = 1;
	float minE = -1000;
	while (numsteps < maxsteps)
	{
		float curstep = numsteps + stride;
		hitpoint = origin + dir * curstep * steplen;


		float3 samplevec = normalize(hitpoint - probepos);
		float hitdist = length(hitpoint - probepos);
		float distance = envdepthTex.SampleLevel(pointClamp, samplevec, 0).r;
		float e = distance - hitdist;
		if ( e < 0)
		{
			if (stride <= 1)
			{
				numsteps += 1;
				//if (e > minE)
				//{
					return true;
				//}
				//minE = e;
			}
			else
				stride = max(1, stride / 2);
		}
		else
		{
			numsteps = curstep;
			stride = stride * 2;
		}
	}

	return false;
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

	//if (!testBox(worldpos.xyz, infmin, infmax)) 
	//	return 0;

	float3 V = normalize(campos - worldpos.xyz);
	float3 R = normalize(reflect(-V, N));

	float3 result  = 0;

	float3 coord = R;
	float3 calcolor = 0;
#if CORRECTED
	float3 corrected = intersectBox(worldpos.xyz, R, boxmin, boxmax);
	coord = normalize(corrected - probepos); 
#elif DEPTH_CORRECTED

	RayParams rp;
	rp.origin = worldpos + N * 0.1f;
	rp.dir = R;
	rp.jitter = 0;

	if (!raymarch(rp,  coord))
		return 0;

	coord = normalize(coord - probepos);
#endif


#if IBL
	float3 lut = LUT(N, V, roughness, lutTexture, sampLinear);
	float3 prefiltered = prefilteredTexture.SampleLevel(sampLinear, coord, roughness * (PREFILTERED_MIP_LEVEL - 1)).rgb;
	float3 irradiance = irradianceTexture.SampleLevel(sampLinear, N, 0).rgb;
	calcolor = indirectBRDF(irradiance, prefiltered, lut, roughness, metallic, F0_DEFAULT, albedo, N, V);
	result = calcolor * intensity /** float3((i % 2), 0, (1.0f - i % 2)) */; 
#elif SH
	float basis[NUM_COEFS];
	HarmonicBasis(N, basis); 
	for (int i = 0; i < NUM_COEFS; ++i)
	{
		result += coefs[i] * basis[i];
	}
	result *= albedo * intensity; 
#else
	calcolor = envTex.SampleLevel(sampLinear, coord, 0).rgb;
	result = calcolor * intensity * (1 - roughness) * metallic  ;
#endif



	return float4(result, 1);
}


