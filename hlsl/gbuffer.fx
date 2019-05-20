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
	float4 WorldPos: TEXCOORD1;
};

GBufferVertexShaderOutput vs(GBufferVertexShaderInput input)
{
	GBufferVertexShaderOutput output = (GBufferVertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 1.0f), World);
	output.WorldPos = worldPosition;
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);

	output.Normal = input.Normal;

	output.TexCoord = input.TexCoord;

	return output;
}

struct GBufferPixelShaderOutput
{
	float4 Color : COLOR0;
	float2 Normal : COLOR1;
	float4 WorldPos: COLOR2;
};

#define kPI 3.1415926536f
float2 encode(float3 n)
{
	float tan = atan2(n.y, n.x);
	return float2(tan / kPI, n.z);
}
GBufferPixelShaderOutput ps(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;
	output.Color = diffuseTex.Sample(sampLinear, input.TexCoord);
	output.Color.a = 1.0f;
	float3 normalFromMap = input.Normal;
	normalFromMap = mul(normalFromMap, World);
	output.Normal = encode(normalize(normalFromMap));
	output.WorldPos = input.WorldPos;
	return output;
}

GBufferPixelShaderOutput ps_notex(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;
	output.Color = float4(1, 1, 1,1);
	float3 normalFromMap = input.Normal;
	normalFromMap = mul(normalFromMap, World);
	output.Normal = encode(normalize(normalFromMap));
	output.WorldPos = input.WorldPos;
	return output;
}

technique11 has_texture
{
	pass 
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}


}

technique11 no_texture
{
	pass noTexture
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps_notex()));
	}
}
