Texture2D lightmap: register(t0);
SamplerState sampLinear: register(s0);

cbuffer ConstantBuffer: register(b0)
{
	matrix LightViewProjection;
}


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
	float3 worldPos:TEXCOORD1;
};


float4 main(PS_INPUT input) : SV_TARGET
{
	float3 pos = mul(input.worldPos, LightViewProjection);

	pos.xy = (pos.xy + 1) * 0.5;
	float depth = lightmap.Sample(sampLinear, pos.xy).r;
	if (depth < pos.z)
		return float4(0, 0, 0, 1);
	else
		return float4(1, 1, 1, 1);
}


