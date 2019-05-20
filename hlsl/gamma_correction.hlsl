Texture2D frameTex: register(t0);
SamplerState PointWrap : register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float3 LinearTosRGB(in float3 color)
{
	float3 x = color * 12.92f;
	float3 y = 1.055f * pow(saturate(color), 1.0f / 2.4f) - 0.055f;

	float3 clr = color;
	clr.r = color.r < 0.0031308f ? x.r : y.r;
	clr.g = color.g < 0.0031308f ? x.g : y.g;
	clr.b = color.b < 0.0031308f ? x.b : y.b;

	return clr;
}


float4 main(PS_INPUT Input) : SV_TARGET
{
	float4 color = frameTex.Sample(PointWrap, Input.Tex);
	//return pow(color, 1.0f / GAMMA);
	color.rgb = LinearTosRGB(color.rgb);
	return color;
}


