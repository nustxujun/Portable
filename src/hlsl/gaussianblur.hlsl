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
	float4 result = frameTex.Sample(smp, coords) * weight[0];

#if (HORIZONTAL)
	{
		for (int i = 1; i < 5; ++i)
		{
			result += frameTex.Sample(smp, coords, int2(i, 0)) * weight[i];
			result += frameTex.Sample(smp, coords, int2(-i, 0)) * weight[i];
		}
	}
#else
	{
		for (int i = 1; i < 5; ++i)
		{
			result += frameTex.Sample(smp, coords, int2(0, i)) * weight[i];
			result += frameTex.Sample(smp, coords, int2(0, -i)) * weight[i];
		}
	}
#endif
	return result;
}