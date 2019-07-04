cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	matrix lastWorld;
}

SamplerState sampLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;
};

struct VertexShaderInput
{
	float3 Position : POSITION0;
};

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 Pos_ss:TEXCOORD0;
	float3 lastPos_ss: TEXCOORD1;
};

VertexShaderOutput vs(VertexShaderInput input)
{
	VertexShaderOutput output = (VertexShaderOutput)0;

	output.Position = mul(mul(mul(float4(input.Position, 1), World), View), Projection);
	output.Pos_ss = output.Position.xyz / output.Position.w;
	float4 lp = mul(mul(mul(float4(input.Position, 1), lastWorld), View), Projection);
	output.lastPos_ss = lp.xyz / lp.w;
	return output;
}

float2 ps(VertexShaderOutput input) :SV_TARGET
{
	return input.Pos_ss.xy - input.lastPos_ss.xy;
}

technique11 dynamic
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}