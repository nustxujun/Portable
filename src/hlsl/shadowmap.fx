
cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 campos;
	float depthscale;
	float near;
	float C;
}


struct VS_INPUT
{
	float3 Pos: POSITION;
};


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos:TEXCOORD0;
	float DepthLinear : TEXCOORD1;
};


PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = float4(input.Pos.xyz, 1.0f);
	output.WorldPos = mul(output.Pos, World);
	output.Pos = mul(output.WorldPos, View);
	output.DepthLinear = output.Pos.z;
	output.Pos = mul(output.Pos, Projection);

	return output;
}


float4 direxp(PS_INPUT input):SV_TARGET
{
	float depth = (input.DepthLinear - near) * depthscale;
	return float4(exp(C * depth), depth, 0, 0);
}


float4 pointps(PS_INPUT input):SV_TARGET
{
	return length(campos.xyz - input.WorldPos.xyz);
}

float4 pointexp(PS_INPUT input) : SV_TARGET
{
	float depth =  length(campos.xyz - input.WorldPos.xyz) * depthscale;
	return exp(C * depth);
//return depth;
}

technique11 DirectionalLight
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(NULL);
	}
}

technique11 DirectionalLightExp
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, direxp()));
	}
}

technique11 PointLight
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, pointps()));
	}
}

technique11 PointLightExp
{
	pass
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, pointexp()));
	}
}