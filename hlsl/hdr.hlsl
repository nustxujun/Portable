Texture2D frameTex: register(t0);
Texture2D lumTex: register(t1);
SamplerState PointWrap : register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


static const float KeyValue = 0.5f;

float Log2Exposure(in float avgLuminance)
{
	float exposure = 0.0f;

	avgLuminance = max(avgLuminance, 0.00001f);
	float linearExposure = (KeyValue / avgLuminance);
	exposure = log2(max(linearExposure, 0.00001f));
	return exposure;
}

float3 CalcExposedColor(in float3 color, in float avgLuminance, in float offset, out float exposure)
{
	exposure = Log2Exposure(avgLuminance);
	exposure += offset;
	return exp2(exposure) * color;
}

//float3 ACESToneMapping(float3 color, float avglum)
//{
//	float exposure = 0;
//	color = CalcExposedColor(color, avglum, 0, exposure);
//
//	const float A = 2.51f;
//	const float B = 0.03f;
//	const float C = 2.43f;
//	const float D = 0.59f;
//	const float E = 0.14f;
//	return (color * (A * color + B)) / (color * (C * color + D) + E);
//}



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
	color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = mul(ACESOutputMat, color);

	// Clamp to [0, 1]
	color = saturate(color);

	return color;
}

float3 tonemappnig(float3 color, float avglum)
{
	float exposure = 0;
	color = CalcExposedColor(color, avglum, 0, exposure);
	return ACESFitted(color);
}

float4 main(PS_INPUT Input) : SV_TARGET
{
	//float4 vColor = 0;
	float4 vColor = frameTex.Sample(PointWrap, Input.Tex);
	float vLum = lumTex.Sample(PointWrap, float2(0,0)).r;
	//float3 vBloom = s2.Sample(LinearSampler, Input.Tex);

	// Tone mapping
	//vColor.rgb *= MIDDLE_GRAY / (vLum.r + 0.001f);
	//vColor.rgb *= (1.0f + vColor / LUM_WHITE);
	//vColor.rgb /= (1.0f + vColor);

	vColor.rgb = tonemappnig(vColor.rgb, vLum);


	//vColor.rgb += 0.6f * vBloom;
	//vColor.a = 1.0f;

	return vColor;
}


