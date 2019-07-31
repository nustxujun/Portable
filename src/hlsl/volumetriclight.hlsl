#include "utilities.hlsl"
#include "shadowmap_utility.hlsl"

Texture2D depthTex: register(t0);
Texture2D shadowmap:register(t1);


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
	float G;
	float3 lightdir;
	float maxlength;
	float density;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

float MieScattering(float cosAngle)
{
	float GG = G * G;
	return (1  / (4 * PI)) *((1 - GG) / pow(1 + GG - 2 * G * cosAngle, 1.5));
}

float4 scatter(float3 accColor, float accTrans, float atten, float density)
{
	density = max(density, 0.0001f);
	float transmittance = exp(-density / numsamples);

	float3 lightintegral = atten * (1 - transmittance) / density;
	accColor += lightintegral * accTrans;
	accTrans *= transmittance;
	return float4(accColor, accTrans);
}

float4 raymarch(float3 start, float3 end)
{
	float4 color = float4(0,0,0,1);
	float3 dir = normalize(end - start);
	float cosAngle = dot(lightdir, -dir);
	float len = length(end - start);
	float rate = len / maxlength * density;
	float step = len / (float)(numsamples + 1 );

	ShadowMapParams smp;
	smp.lightview = lightView;
	//smp.lightProjs = lightProjs;
	{
		for (int i = 0; i < numcascades; ++i)
		{
			smp.lightProjs[i] = lightProjs[i];
		}
	}
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

	for (int i = 1; i <= numsamples; ++i)
	{
		float3 cur = start + dir * step * i;
		float atten = MieScattering(cosAngle);
		
		smp.worldpos = cur;
		smp.depth_viewspace = mul(float4(cur,1), view).z;

		int index = 0;
		atten *= getShadow(smp, index);
		color = scatter(color.rgb, color.a, atten, rate);

	}

	color.rgb *= lightcolor;
	color.a = saturate(color.a);

	return color;
}

float4 main(PS_INPUT input) : SV_TARGET
{
	float2 uv = input.Tex;
	float depth = depthTex.SampleLevel(samp, uv, 0).r;
	float3 worldpos = getWorldPos(uv, depth, invertViewProj);

	float4 color = raymarch(campos, worldpos);

	return color;
}


