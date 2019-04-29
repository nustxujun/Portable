
cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

Texture2D colorTexture: register(t0);
SamplerState sampLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};


struct VS_INPUT
{
	float3 Pos: POSITION;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = float4(input.Pos.xyz, 1.0f);
	output.Pos = mul(mul(mul(output.Pos, World), View), Projection);
	output.Tex = input.Tex;

	return output;
}




float4 ps(PS_INPUT input) : SV_TARGET
{
	//return float4(input.Tex,0,0);
	return colorTexture.Sample(sampLinear, input.Tex);
}

technique11 test
{
	pass pass1
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));

	}
}

