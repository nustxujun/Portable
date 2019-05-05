
Texture2D lightmap: register(t0);
SamplerState sampLinear: register(s0);

cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	matrix LightViewProjection;
}



struct VS_INPUT
{
	float3 Pos: POSITION;
};


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float4 worldPos:TEXCOORD0;
};

PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = float4(input.Pos.xyz, 1.0f);
	output.worldPos = output.Pos;

	output.Pos = mul(mul(mul(output.Pos, World), View), Projection);

	return output;
}



float4 ps(PS_INPUT input) : SV_TARGET
{
	float4 pos = mul(input.worldPos, LightViewProjection);

	pos.x = (pos.x + 1) * 0.5;
	pos.y =  ( 1 - pos.y ) * 0.5;
	float depth = lightmap.Sample(sampLinear, pos.xy).r;

	//return float4( 1 - depth, 1 - depth, 1 - depth, 1);
	//return float4( 1 - pos.z,  1- pos.z,1- pos.z, 1);

	if (depth + 0.00001 < pos.z)
		return float4(0.2,0.2,0.2,1);
	else
		return float4(1,1,1,1);
}


technique11 
{
	pass 
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}

