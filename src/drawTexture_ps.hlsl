Texture2D colorTexture: register(t0);
SamplerState sampLinear: register(s0);



struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT input) : SV_TARGET
{
	return colorTexture.Sample(sampLinear, input.Tex);
}


