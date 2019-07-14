Buffer<float4> lights: register(t0);

cbuffer ConstantBuffer: register(b0)
{
	matrix View;
	matrix Projection;
	int stride;
}
struct VS_INPUT
{
	float3 Pos: POSITION;
	uint index : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	uint index : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = lights[input.index * stride];

	pos = float4(input.Pos.xyz * pos.w + pos.xyz, 1);
	output.Pos = mul(mul(pos, View), Projection);
	output.index = input.index;
	return output;
}


