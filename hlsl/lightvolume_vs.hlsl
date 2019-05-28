cbuffer ConstantBuffer: register(b0)
{
	matrix View;
	matrix Projection;
}
struct VS_INPUT
{
	float3 Pos: POSITION;
	float4 posAndRange : TEXCOORD0;
	float index : TEXCOORD1;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
	uint index : TEXCOORD1;
};

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = float4(input.Pos.xyz * input.posAndRange.w + input.posAndRange.xyz, 1);
	output.Pos = mul(mul(pos, View), Projection);

	float2 clippos = output.Pos.xy / output.Pos.w;
	output.Tex = float2(clippos.x * 0.5f + 0.5f, 0.5f - clippos.y* 0.5f);
	output.index = (uint)input.index;
	return output;
}


