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

struct GBufferVertexShaderInput
{
	float3 Position : POSITION0;
};

struct GBufferVertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 TexCoord : TEXCOORD0;
};

GBufferVertexShaderOutput vs(GBufferVertexShaderInput input)
{
	GBufferVertexShaderOutput output = (GBufferVertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 0.0f), World);
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);
	
	output.TexCoord = normalize( worldPosition.xyz);

	output.Position.z = output.Position.w;


	return output;
}

struct GBufferPixelShaderOutput
{
	float4 Color : COLOR0;
};

GBufferPixelShaderOutput ps(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;
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
