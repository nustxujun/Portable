Texture2D frameTex: register(t0);
SamplerState sampPoint: register(s0);

static const float4 LUM_VECTOR = float4(.299, .587, .114, 0);


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 downSample2x2(PS_INPUT Input): SV_TARGET
{
	float4 vColor = 0.0f;
	float  fAvg = 0.0f;

	for (int y = -1; y < 1; y++)
	{
		for (int x = -1; x < 1; x++)
		{
			// Compute the sum of color values
			vColor = frameTex.Sample(sampPoint, Input.Tex, int2(x,y));

			fAvg += dot(vColor.r, LUM_VECTOR);
		}
	}

	fAvg /= 4;

	return float4(fAvg, fAvg, fAvg, 1.0f);
}

float4 downSample3x3(PS_INPUT Input) : SV_TARGET
{
	float fAvg = 0.0f;
	float4 vColor;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			// Compute the sum of color values
			vColor = frameTex.Sample(sampPoint, Input.Tex, int2(x,y));

			fAvg += vColor.r;
		}
	}

	// Divide the sum to complete the average
	fAvg /= 9;

	return float4(fAvg, fAvg, fAvg, 1.0f);
}