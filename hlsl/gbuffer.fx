#if !(UV) && (ALBEDO || NORMAL_MAP || AO_MAP || HEIGHT_MAP)
#error Need coords for textures(albedo normal roughness metallic ao ...)
#endif

cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float roughness;
	float metallic;
	float reflection;
};

cbuffer ParallaxConstants:register(b1)
{
	float3 campos;
	float heightscale;
	uint minsamplecount;
	uint maxsamplecount;
};
Texture2D diffuseTex: register(t0);
Texture2D normalTex:register(t1);
Texture2D roughTex:register(t2);
Texture2D metalTex:register(t3);
Texture2D aoTex:register(t4);
Texture2D heightTex:register(t5);

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
#if NORMAL_MAP || HEIGHT_MAP
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
#if HEIGHT_MAP
	float3 PosInTangent:TEXCOORD1;
	float3 CamInTangent:TEXCOORD2;
	float3 WorldPosition:TEXCOORD3;
#endif
};

struct GBufferPixelShaderOutput
{
	float4 Color : COLOR0;
	float4 Normal : COLOR1;
	float4 Material: COLOR2;
};

GBufferVertexShaderOutput vs(GBufferVertexShaderInput input)
{
	GBufferVertexShaderOutput output = (GBufferVertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 1.0f), World);
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);

	output.Normal = mul(input.Normal, (float3x3)World).xyz;
#if UV
	output.TexCoord = input.TexCoord;
#endif

#if NORMAL_MAP || HEIGHT_MAP
	float3 tangent = mul(input.Tangent, (float3x3)World);
	float3 bitangent = mul(input.Bitangent, (float3x3)World);
#endif

#if NORMAL_MAP
	output.Tangent = tangent;
	output.Bitangent = bitangent;
#endif

#if HEIGHT_MAP
	float3x3 TBN = float3x3(tangent, bitangent, output.Normal);
	float3x3 toTBN = transpose(TBN);
	output.PosInTangent = mul(worldPosition.xyz, toTBN);
	output.CamInTangent = mul(campos, toTBN);
	output.WorldPosition = worldPosition.xyz;
#endif
	return output;
}


#if HEIGHT_MAP
float2 ParallaxMapping(float2 coord, float3 V, int sampleCount)
{
	float2 maxParallaxOffset = -V.xy * heightscale / V.z;
	float zStep = 1.0f / (float)sampleCount;
	float2 texStep = maxParallaxOffset * zStep;
	float2 dx = ddx(coord);
	float2 dy = ddy(coord);
	int sampleIndex = 0;
	float2 currTexOffset = 0;
	float2 prevTexOffset = 0;
	float2 finalTexOffset = 0;
	float currRayZ = 1.0f - zStep;
	float prevRayZ = 1.0f;
	float currHeight = 0.0f;
	float prevHeight = 0.0f;
	// Ray trace the heightfield.
	while (sampleIndex < sampleCount + 1)
	{
		currHeight = heightTex.SampleGrad(sampLinear,
			coord + currTexOffset, dx, dy).r;
		// Did we cross the height profile?
		if (currHeight > currRayZ)
		{
			// Do ray/line segment intersection and compute final tex offset.
			float t = (prevHeight - prevRayZ) /
				(prevHeight - currHeight + currRayZ - prevRayZ);
			finalTexOffset = prevTexOffset + t * texStep;
			// Exit loop.
			break;
		}
		else
		{
			++sampleIndex;
			prevTexOffset = currTexOffset;
			prevRayZ = currRayZ;
			prevHeight = currHeight;
			currTexOffset += texStep;
			// Negative because we are going "deeper" into the surface.
			currRayZ -= zStep;
		}
	}
	return coord + finalTexOffset;
	
}
#endif


GBufferPixelShaderOutput ps(GBufferVertexShaderOutput input) : SV_TARGET
{
	GBufferPixelShaderOutput output;
#if UV
	float2 coord = input.TexCoord;
#endif
#if HEIGHT_MAP
	float3 V = normalize(input.CamInTangent - input.PosInTangent);
	uint sampleCount = lerp(minsamplecount, maxsamplecount, dot(input.Normal, normalize(campos - input.WorldPosition)));
	coord = ParallaxMapping(coord, V, sampleCount) ;
#endif

#if ALBEDO
	output.Color = diffuseTex.Sample(sampLinear, coord);
#if !(SRGB)
	output.Color.rgb = pow(output.Color.rgb, 2.2f);
#endif
	output.Color.a = 1.0f;
#else
	output.Color = 1.0f;
#endif

#if AO_MAP
	output.Color.rgb *= aoTex.Sample(sampLinear, coord).rgb;
#endif

#if NORMAL_MAP 
	float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Normal);
	float3 normal = normalize(normalTex.Sample(sampLinear, coord).rgb * 2.0f - 1.0f);
	output.Normal.xyz = mul(normal, TBN);
#else
	output.Normal.xyz = input.Normal;
#endif

#if PBR_MAP
	float r = roughTex.Sample(sampLinear, coord).r;
	float m = metalTex.Sample(sampLinear, coord).r;
	output.Material = float4(r * roughness, m * metallic, reflection, 0);
#else
	output.Material = float4(roughness, metallic, reflection, 0);
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

