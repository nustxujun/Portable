cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

Texture2D diffuseTex: register(t0);

SamplerState sampLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct GBufferVertexShaderInput
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
	float2 TexCoord : TEXCOORD0;
};

struct GBufferVertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL0;
	float2 TexCoord : TEXCOORD0;
};

GBufferVertexShaderOutput vs(GBufferVertexShaderInput input)
{
	GBufferVertexShaderOutput output = (GBufferVertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 1.0f), World);
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);

	output.Normal = input.Normal;

	output.TexCoord = input.TexCoord;

	return output;
}

struct GBufferPixelShaderOutput
{
	float4 Color : COLOR0;
	float4 Normal : COLOR1;
};

GBufferPixelShaderOutput ps(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;
	output.Color = diffuseTex.Sample(sampLinear, input.TexCoord);
	output.Color.a = 1.0f;
	float3 normalFromMap = input.Normal;
	normalFromMap = mul(normalFromMap, World);
	normalFromMap = normalize(normalFromMap);
	output.Normal.rgb = 0.5f * (normalFromMap + 1.0f);
	output.Normal.a = 1.0f;

	return output;
}


technique11 albedo_only
{
	pass pass1
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}

