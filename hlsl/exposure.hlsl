Texture2D frameTex: register(t0);
SamplerState PointWrap : register(s0);

cbuffer ConstantBuffer: register(b0)
{
	float keyvalue;
	float lumminclamp;
	float lummaxclamp;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

#define EPSILON         1.0e-4

float Exposure(in float avgLuminance)
{
	float exposure = 0.0f;
	avgLuminance = min(avgLuminance, lummaxclamp);
	avgLuminance = max(avgLuminance, lumminclamp);


	avgLuminance = max(avgLuminance, EPSILON);
	float linearExposure = (keyvalue / avgLuminance);
	exposure = (max(linearExposure, EPSILON));
	return exposure;
}


float4 main(PS_INPUT Input) : SV_TARGET
{
	float4 color = frameTex.Sample(PointWrap, Input.Tex);

	return Exposure(color.r);
}


