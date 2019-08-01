cbuffer Constants: register(c0)
{
	float3 campos;
	float farZ;
}


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos:TEXCOORD0;
};


float main(PS_INPUT input):SV_TARGET
{
	return length(campos - input.WorldPos.xyz);
}
