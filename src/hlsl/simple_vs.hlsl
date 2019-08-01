
cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}


struct VS_INPUT
{
	float3 Pos: POSITION;
};


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos:TEXCOORD0;
};


PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = float4(input.Pos.xyz, 1.0f);
	output.WorldPos = mul(output.Pos, World);
	output.Pos = mul(output.WorldPos, View);
	output.Pos = mul(output.Pos, Projection);

	return output;
}
