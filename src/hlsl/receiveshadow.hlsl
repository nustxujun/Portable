Texture2D depthTex: register(t0);
Texture2D shadowmapTex: register(t1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertView;
	matrix invertProj;
	matrix lightView;
	matrix lightProjs[8];
	float4 cascadeDepths[8];
	int numcascades;
	float scale;
	float shadowcolor;
	float depthbias;
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
		return  percentlit * 0.04f * (1 - shadowcolor) + shadowcolor;
	}

	return 1;
}


