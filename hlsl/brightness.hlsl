Texture2D frameTex: register(t0);
SamplerState PointWrap : register(s0);

cbuffer ConstantBuffer: register(b0)
{
	float threshold;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT Input) : SV_TARGET
{
	float4 color = frameTex.Sample(PointWrap, Input.Tex);


	// Check whether fragment output is higher than threshold, if so output as brightness color
	float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
	if (brightness > threshold)
		return color;
	else
		return 0;
}


