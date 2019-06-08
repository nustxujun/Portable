Texture2D frameTex: register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


const static float weight[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };


float4 main(PS_INPUT input) : SV_TARGET
{
	float2 coords = input.Tex;
	float3 result = frameTex.Sample(smp, coords).rgb * weight[0];

#if (HORIZONTAL)
	{
		for (int i = 1; i < 5; ++i)
		{
			result += frameTex.Sample(smp, coords, uint2(i, 0)).rgb * weight[i];
			result += frameTex.Sample(smp, coords, uint2(-i, 0)).rgb * weight[i];
		}
	}
#else
	{
		for (int i = 1; i < 5; ++i)
		{
			result += frameTex.Sample(smp, coords, uint2(0, i)).rgb * weight[i];
			result += frameTex.Sample(smp, coords, uint2(0, -i)).rgb * weight[i];
		}
	}
#endif
	return float4(result, 1);
}