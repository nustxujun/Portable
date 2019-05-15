Texture2D frameTex: register(t0);
SamplerState linearWrap : register(s0);



struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT input) : SV_TARGET
{
	float4 color = frameTex.Sample(linearWrap, input.Tex);
	float length = normalize(color.xyz);

	return pow(color, 10);
}


