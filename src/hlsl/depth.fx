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
};

struct VertexShaderInput
{
	float3 Position : POSITION0;
};

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 Depth : TEXCOORD0;
};

VertexShaderOutput vs(VertexShaderInput input)
{
	VertexShaderOutput output = (VertexShaderOutput)0;

	float4 worldPosition = mul(float4(input.Position.xyz, 1.0f), World);
	float4 viewPosition = mul(worldPosition, View);
	output.Position = mul(viewPosition, Projection);

	output.Depth.x = output.Position.z;
	output.Depth.y = output.Position.w;

	return output;
}


struct PixelShaderOutput
{
	float4 depth1: COLOR0;
	float4 depth2: COLOR1;
	float4 depth3: COLOR2;
	float4 depth4: COLOR3;

};

PixelShaderOutput ps(VertexShaderOutput input) : SV_TARGET
{
	PixelShaderOutput output;
	float depth = input.Depth.x / input.Depth.y;
	//if (depth < 0.25)
		output.depth1 = depth;
	//else if (depth < 0.5)
	//	output.depth2 = depth;
	//else if (depth < 0.75)
	//	output.depth3 = depth;
	//else
	//	output.depth4 = depth;
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

