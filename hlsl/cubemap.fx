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

static const float PI = 3.14159265359f;

float4 irradianceMap(VertexShaderOutput pin) : SV_TARGET
{
	// Pre-integration
	float3 irradiance = float3(0.0f, 0.0f, 0.0f);

	float3 normal = normalize(pin.TexCoord);
	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = 0.25f;
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

float4 testps(VertexShaderOutput pin) : SV_TARGET
{
	return diffuseTex.Sample(sampLinear, pin.TexCoord);
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


technique11 test
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, testps()));
	}
}
