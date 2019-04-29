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
	//float3 Binormal : BINORMAL0;
	//float3 Tangent : TANGENT0;
};

struct GBufferVertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL0;
	float2 TexCoord : TEXCOORD0;
	float2 Depth : TEXCOORD1;
	//float3x3 tangentToWorld : TEXCOORD2;
};

GBufferVertexShaderOutput vs(GBufferVertexShaderInput input)
{
	GBufferVertexShaderOutput output = (GBufferVertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 1.0f), World);
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);

	output.Normal = input.Normal;

	output.TexCoord = input.TexCoord;

	output.Depth.x = output.Position.z;
	output.Depth.y = output.Position.w;

	// Calculate tangent space to world space matrix using the world space tanget, binormal and normal as basis vector.
	//output.tangentToWorld[0] = mul(float4(input.Tangent, 1.0), World).xyz;
	//output.tangentToWorld[1] = mul(float4(input.Binormal, 1.0), World).xyz;
	//output.tangentToWorld[2] = mul(float4(input.Normal, 1.0), World).xyz;

	return output;
}


struct GBufferPixelShaderOutput
{
	half4 Color : COLOR0;
	half4 Normal : COLOR1;
	half4 Depth: COLOR2;
};

GBufferPixelShaderOutput ps(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;

	//output.Color = float4(1.0f, 1.0f, 1.0f, 1.0f);//tex2D(diffuseSampler, input.TexCoord);
	output.Color = diffuseTex.Sample(sampLinear, input.TexCoord);
	//float4 specularAttributes = tex2D(specularSampler, input.TexCoord);

	// Specular intensity.
	output.Color.a = 0.0f;//specularAttributes.r;

	//// Read the normal from the normal map.
	//float3 normalFromMap = tex2D(normalSampler, input.TexCoord);
	float3 normalFromMap = input.Normal;
	//
	//// Transform to [1, 1].
	//normalFromMap = 2.0f * normalFromMap - 1.0f;

	//// Transform into world space.
	//normalFromMap = mul(normalFromMap, input.tangentToWorld);
	normalFromMap = mul(normalFromMap, World);

	//// Normalize the result.
	normalFromMap = normalize(normalFromMap);

	// Output the normal in [0, 1] space.
	output.Normal.rgb = 0.5f * (normalFromMap + 1.0f);

	// Specular Power.
	output.Normal.a = 1.0f;//specularAttributes.a;

	// Depth.
	output.Depth = input.Depth.x / input.Depth.y;

	return output;
}



technique11 gbuffer
{
	pass pass1
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));

	}
}

