#include "spherical_harmonics.hlsl"
#include "pbr.hlsl"

Texture2D albedoTex:register(t0);
Texture2D depthTex:register(t1);
Texture2D normalTex: register(t2);
Texture2D materialTex:register(t3);
Texture3D coefsTex[COEF_ARRAY_SIZE]: register(t4);

cbuffer Constants:register(b0)
{
	matrix invertViewProj;
	float3 origin;
	float intensity;
	float3 range;
	float3 campos;
}

SamplerState sampLinear:register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

void getCoefficients(float3 uv, out float3 coefs[NUM_COEFS])
{
	float4 C[COEF_ARRAY_SIZE];
	for (int i = 0; i < COEF_ARRAY_SIZE; ++i)
	{
		C[i] = coefsTex[i].SampleLevel(sampLinear, uv, 0);
		//C[i] = coefsTex[i].Load(int4(uv,0));
		//C[i] = coefsTex[i].Load(int4(0,1,0,0));

	}

	coefs[0] = C[0].rgb;
#if (DEGREE >= 1)
	coefs[1] = float3(C[0].a, C[1].rg);
	coefs[2] = float3(C[1].ba, C[2].r);
	coefs[3] = float3(C[2].gba);
#endif

#if (DEGREE >= 2)
	coefs[4] = float3(C[3].rgb);
	coefs[5] = float3(C[3].a, C[4].rg);
	coefs[6] = float3(C[4].ba, C[5].r);
	coefs[7] = float3(C[5].gba);
	coefs[8] = float3(C[6].rgb);
#endif

#if (DEGREE >= 3)
	coefs[9] = float3(C[6].a, C[7].rg);
	coefs[10] = float3(C[7].ba, C[8].r);
	coefs[11] = float3(C[8].gba);
	coefs[12] = float3(C[9].rgb);
	coefs[13] = float3(C[9].a, C[10].rg);
	coefs[14] = float3(C[10].ba, C[11].r);
	coefs[15] = float3(C[11].gba);
#endif
}

float4 main(PS_INPUT Input) : SV_TARGET
{
	float3 albedo = albedoTex.SampleLevel(sampLinear, Input.Tex,0).rgb;
	float3 N = normalize(normalTex.SampleLevel(sampLinear, Input.Tex, 0).rgb);
	float4 material = materialTex.SampleLevel(sampLinear, Input.Tex, 0);
	float depth = depthTex.SampleLevel(sampLinear, Input.Tex, 0).r;
	float4 worldpos;
	worldpos.x = Input.Tex.x * 2.0f - 1.0f;
	worldpos.y = -(Input.Tex.y * 2.0f - 1.0f);
	worldpos.z = depth;
	worldpos.w = 1.0f;

	worldpos = mul(worldpos, invertViewProj);
	worldpos /= worldpos.w;

	float3 relpos = worldpos.xyz - origin;
	float3 uv = relpos / range.xyz ;

	float basis[NUM_COEFS];
	HarmonicBasis(N, basis);
	float3 coefs[NUM_COEFS];
	getCoefficients(uv, coefs);

	float3 color = 0;
	for (int i = 0; i < NUM_COEFS; ++i)
	{
		color += coefs[i] * basis[i];
	}

	float metallic = material.g;
	float roughness = material.r;

	float3 V = normalize(campos - worldpos.xyz);
	float3 f0 = lerp(F0_DEFAULT, albedo, metallic);
	float3 kS = FresnelSchlickRoughness(V, N, f0, roughness);
	float3 kD = 1.0f - kS;
	kD *= (1.0f - metallic);

	color = saturate(color) * kD * albedo * intensity;

	return float4(color, 1);
}