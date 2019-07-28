Texture2D depthTex: register(t0);
Texture2D shadowmapTex: register(t1);
Texture2D normalTex:register(t2);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertView;
	matrix invertProj;
	matrix lightView;
	matrix lightProjs[8];
	float4 cascadeDepths[8];
	float3 lightdir;
	int numcascades;
	float scale;
	float shadowcolor;
	float depthbias;
	float translucency;
	float translucency_bias;
}
SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);
SamplerComparisonState sampshadow : register(s2);


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};



float4 ps(PS_INPUT input) : SV_TARGET
{
	float3 N = normalTex.SampleLevel(sampPoint, input.Tex,0).rgb;
	N = normalize(N);
	float depth = depthTex.Sample(sampPoint, input.Tex).r;
	float4 worldPos = 0;
	worldPos.x = input.Tex.x * 2.0f - 1.0f;
	worldPos.y = -(input.Tex.y * 2.0f - 1.0f);
	worldPos.z = depth;
	worldPos.w = 1.0f;
	worldPos = mul(worldPos, invertProj);
	worldPos /= worldPos.w;
	float depthlinear = worldPos.z;
	worldPos = mul(worldPos, invertView);

	//worldPos = float4(worldPos - 0.005 * N, 1.0);

	[unroll]
	for (int i = 0; i < numcascades; ++i)
	{
		if (depthlinear > cascadeDepths[i].r)
			continue;


		float4 pos = mul(worldPos, lightView);
		pos = mul(pos, lightProjs[i]);
		pos /= pos.w;

		float cmpdepth = pos.z - depthbias;

		pos.x = (pos.x + 1) * 0.5;
		pos.y = (1 - pos.y) * 0.5;

		float2 uv = (pos.xy + float2(i, 0)) * float2(scale, 1);
		//float sampledepth = shadowmapTex.Sample(sampLinear, uv).r;
		//if ( sampledepth < 1.0f && sampledepth + depthbias < pos.z)
		//	return shadowcolor;
		//else
		//	return 1;
		float percentlit = 0.0f;
		for (int x = -2; x <= 2; ++x)
		{
			for (int y = -2; y <= 2; ++y)
			{
				percentlit += shadowmapTex.SampleCmpLevelZero(sampshadow, uv, cmpdepth, int2(x, y));
			}
		}
#if TRANSMITTANCE
		float4 shrinkpos = float4(worldPos.xyz - N * translucency_bias, 1.0);
		shrinkpos = mul(shrinkpos, lightView);
		shrinkpos = mul(shrinkpos, lightProjs[i]);
		shrinkpos /= pos.w;
		shrinkpos.x = shrinkpos.x * 0.5 + 0.5;
		shrinkpos.y = 0.5 - shrinkpos.y * 0.5;

		uv = (shrinkpos.xy + float2(i, 0)) * float2(scale, 1);

		float d1 = shadowmapTex.Sample(sampLinear, uv).r;
		float d2 = shrinkpos.z;
		float d =  (1 - translucency) * abs(d1 - d2) * cascadeDepths[i].r * 1000;
		float dd = -d * d;
		float3 profile = float3(0.233, 0.455, 0.649) * exp(dd / 0.0064) +
			float3(0.1, 0.336, 0.344) * exp(dd / 0.0484) +
			float3(0.118, 0.198, 0.0)   * exp(dd / 0.187) +
			float3(0.113, 0.007, 0.007) * exp(dd / 0.567) +
			float3(0.358, 0.004, 0.0)   * exp(dd / 1.99) +
			float3(0.078, 0.0, 0.0)   * exp(dd / 7.41);
		return float4(profile * saturate(0.3 + dot(-N, -lightdir)), percentlit * 0.04f * (1 - shadowcolor) + shadowcolor);
#else
		return  percentlit * 0.04f * (1 - shadowcolor) + shadowcolor;
#endif
	}
#if TRANSMITTANCE
	return float4(0, 0, 0, 1);
#else
	return 1;
#endif
}


