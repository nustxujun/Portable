TextureCube diffuseTex: register(t0);

cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}


SamplerState sampLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

struct VertexShaderInput
{
	float3 Position : POSITION0;
};

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 TexCoord : TEXCOORD0;
};

VertexShaderOutput vs(VertexShaderInput input)
{
	VertexShaderOutput output = (VertexShaderOutput)0;

	float4 localPos =float4(input.Position.xyz, 0.0f);
	float4 viewPosition = mul(localPos, View);
	output.Position = mul(viewPosition, Projection).xyww;
	output.TexCoord = normalize(localPos.xyz);
	return output;
}

struct PixelShaderOutput
{
	float4 Color : COLOR0;
};

PixelShaderOutput ps(VertexShaderOutput input) : SV_TARGET
{
	PixelShaderOutput output;
	output.Color = diffuseTex.Sample(sampLinear, input.TexCoord);
	output.Color.rgb = pow(output.Color.rgb, 2.2f);
	output.Color.a = 1.0f;
	return output;
}


technique11 
{
	pass 
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}
