Texture2D frameTex: register(t0);
Texture2D exposureTex: register(t1);
SamplerState PointWrap : register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


static const float3x3 ACESInputMat =
{
	{0.59719, 0.35458, 0.04823},
	{0.07600, 0.90834, 0.01566},
	{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367},
	{-0.10208,  1.10813, -0.00605},
	{-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 ACESFitted(float3 color)
{
	//color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	//color = mul(ACESOutputMat, color);

	// Clamp to [0, 1]
	color = saturate(color);

	return color;
}

float3 tonemappnig(float3 color, float exp)
{
	return ACESFitted(color * exp);
}

float4 main(PS_INPUT Input) : SV_TARGET
{
	float4 vColor = frameTex.Sample(PointWrap, Input.Tex);
	float exp = exposureTex[uint2(0,0)].r;

	vColor.rgb = tonemappnig(vColor.rgb, exp);


	return vColor;
}


