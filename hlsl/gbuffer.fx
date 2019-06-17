cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float roughness;
	float metallic;
}

Texture2D diffuseTex: register(t0);
Texture2D normalTex:register(t1);
Texture2D roughTex:register(t2);
Texture2D metalTex:register(t3);
Texture2D aoTex:register(t4);

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
#if UV
	float2 TexCoord : TEXCOORD0;
#endif
#if NORMAL_MAP
	float3 Tangent :TANGENT0;
	float3 Bitangent: TANGENT1;
#endif
};

struct GBufferVertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL0;
#if UV
	float2 TexCoord : TEXCOORD0;
#endif
#if NORMAL_MAP
	float3 Tangent: TANGENT0;
	float3 Bitangent: TANGENT1;
#endif
};

struct GBufferPixelShaderOutput
{
	float4 Color : COLOR0;
	float4 Normal : COLOR1;
	float2 Material: COLOR2;
};

GBufferVertexShaderOutput vs(GBufferVertexShaderInput input)
{
	GBufferVertexShaderOutput output = (GBufferVertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 1.0f), World);
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);

	output.Normal = mul(float4(input.Normal,0), World).xyz;
#if ALBEDO
	output.TexCoord = input.TexCoord;
#endif

#if NORMAL_MAP
	output.Tangent = mul(input.Tangent, (float3x3)World);
	output.Bitangent = mul(input.Bitangent, (float3x3)World);
#endif
	return output;
}




GBufferPixelShaderOutput ps(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;
#if ALBEDO
	output.Color = diffuseTex.Sample(sampLinear, input.TexCoord);
#if !(SRGB)
	output.Color.rgb = pow(output.Color.rgb, 2.2f);
#endif
	output.Color.a = 1.0f;
#else
	output.Color = 1.0f;
#endif

#if AO_MAP
	output.Color.rgb *= aoTex.Sample(sampLinear, input.TexCoord).rgb;
#endif

#if NORMAL_MAP 
	float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Normal);
	float3 normal = normalize(normalTex.Sample(sampLinear, input.TexCoord).rgb * 2.0f - 1.0f);
	output.Normal.xyz = mul(normal, TBN);
#else
	output.Normal.xyz = input.Normal;
#endif

#if PBR_MAP
	float r = roughTex.Sample(sampLinear, input.TexCoord).r;
	float m = roughTex.Sample(sampLinear, input.TexCoord).r;
	output.Material = float2(r * roughness, m * metallic);
#else
	output.Material = float2(roughness, metallic);
#endif
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

