TextureCube diffuseTex: register(t0);

cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

cbuffer PrefilterConstants: register(b1)
{
	float roughnessCb;
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
	viewPosition.w = 1.0f;
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

static const float PI = 3.14159265359f;

float4 irradianceMap(VertexShaderOutput pin) : SV_TARGET
{
	// Pre-integration
	float3 irradiance = float3(0.0f, 0.0f, 0.0f);

	float3 normal = normalize(pin.TexCoord);
	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = 0.025f;
	float numSamples = 0.0f;
	for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
			// spherical to cartesian (in tangent space)
			float3 tangentSample = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += diffuseTex.Sample(sampLinear, sampleVec).rgb * cos(theta) * sin(theta);
			numSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0f / numSamples);

	return float4(irradiance, 1.0f);
}

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float4 prefilterMap(VertexShaderOutput pin) : SV_TARGET
{
	float roughness = roughnessCb;
	const uint NumSamples = 1024u;
	float3 N = normalize(pin.TexCoord);
	float3 R = N;
	float3 V = R;

	float3 prefilteredColor = float3(0.f, 0.f, 0.f);
	float totalWeight = 0.f;
	for (uint i = 0u; i < NumSamples; ++i)
	{
		float2 Xi = Hammersley(i, NumSamples);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0f * dot(V, H) * H - V);
		float NdotL = max(dot(N, L), 0.0f);
		if (NdotL > 0.f)
		{
			prefilteredColor += diffuseTex.Sample(sampLinear, L).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	prefilteredColor = prefilteredColor / totalWeight;

	return float4(prefilteredColor, 1.0f);
}



float4 testps(VertexShaderOutput pin) : SV_TARGET
{
	return float4(pin.TexCoord, 1);
}



technique11 skybox
{
	pass 
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}

technique11 irradiance
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, irradianceMap()));
	}
}
technique11 prefilter
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, prefilterMap()));
	}
}


technique11 test
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, testps()));
	}
}
