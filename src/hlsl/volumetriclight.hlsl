#include "utilities.hlsl"
#include "shadowmap_utility.hlsl"

Texture2D depthTex: register(t0);

#if DIR
Texture2D
#elif POINT
TextureCube
#endif
shadowmap:register(t1);

Texture2D noiseTex: register(t2);



SamplerState samp : register(s0);
SamplerComparisonState smpcmp:register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	matrix view;
	matrix lightView;
	matrix lightProjs[8];
	float4 cascadeDepths[2];
	int numcascades;

	float3 campos;
	int numsamples;
	float3 lightcolor;
	float jittervalue;
	float3 lightdir;
	float maxlength;
	float density;
	float2 noiseSize;
	float2 screenSize;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float volumetricShadow(float3 pos)
{
#if DIR


	DirShadowMapParams smp;
	smp.lightview = lightView;
	smp.lightProjs = lightProjs;

	{
		smp.cascadedepths[0] = cascadeDepths[0].r;
		smp.cascadedepths[1] = cascadeDepths[0].g;
		smp.cascadedepths[2] = cascadeDepths[0].b;
		smp.cascadedepths[3] = cascadeDepths[0].a;

		smp.cascadedepths[4] = cascadeDepths[1].r;
		smp.cascadedepths[5] = cascadeDepths[1].g;
		smp.cascadedepths[6] = cascadeDepths[1].b;
		smp.cascadedepths[7] = cascadeDepths[1].a;
	}
	smp.numcascades = numcascades;
	smp.depthbias = 0.001f;
	smp.shadowmap = shadowmap;
	smp.samp = smpcmp;
	// it will cross the mip border which i cannot reslove now
	// using the last cascade
	smp.startcascade = numcascades - 1;
	smp.worldpos = pos;
	smp.depth_viewspace = mul(float4(pos, 1), view).z;

	return getDirShadow(smp);
#elif POINT
	PointShadowMapParams smp;
	smp.depthbias = 0.001f;
	smp.lightpos = lightdir;
	smp.farZ = cascadeDepths[0].r;
	smp.shadowmap = shadowmap;
	smp.samp = smpcmp;
	smp.scale = 1.0f / (float)numcascades;
	smp.worldpos = pos;
	return getPointShadow(smp);
#endif
}

float3 evaluateLight(float3 pos)
{
#if DIR
	float3 L = -lightdir.xyz;
#elif POINT
	float3 L = normalize(lightdir.xyz - pos);
#endif
	return lightcolor   * 1.0 / dot(L, L);
}


float4 raymarch(float3 start, float3 end, float jitter)
{
	float3 dir = normalize(end - start);
	float len = length(end - start);
	float step = maxlength / (float)(numsamples + 1 );

	float sigmaS = density;
	float sigmaE = max(0.000001f, sigmaS);

	float d = step * (1 + jitter);
	float dd = 0;

	float3 scatteredLight = 0;
	float transmittance = 1.0;
	const float phaseFunction = (1.0f / (4.0f * 3.14159265358f));

	for (int i = 0; i < numsamples; ++i)
	{
		float3 p = start + dir * d;

		float3 S = evaluateLight(p) * sigmaS * phaseFunction * volumetricShadow(p);
		float3 Sint = (S - S * exp(-sigmaE * dd)) / sigmaE;

		scatteredLight += transmittance * Sint;
		transmittance *= exp(-sigmaE * dd);

		if (d >= len)
			break;


		dd = step;
		d += dd;
	}



	return float4(scatteredLight, transmittance);
}

float4 main(PS_INPUT input) : SV_TARGET
{
	float2 uv = input.Tex;
	float depth = depthTex.SampleLevel(samp, uv, 0).r;
	float3 worldpos = getWorldPos(uv, depth, invertViewProj);

	int2 noiseuv = uint2((uv)* screenSize) % uint2(noiseSize);
	float2 Xi = noiseTex.Load(int3(noiseuv, 0)).rg;
	Xi *= jittervalue;


	float4 color = raymarch(campos, worldpos, (Xi.x + Xi.y) * 0.5f);
	return color;
}


