
Texture2D lightmap1: register(t0);
Texture2D lightmap2: register(t1);
Texture2D lightmap3: register(t2);
Texture2D lightmap4: register(t3);
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


//float getDepth(float4 pos)
//{
//	float depth = 0;
//	if (pos.z < 0.25f)
//		depth = max(depth, lightmap1.Sample(sampLinear, pos.xy).r);
//	if (pos.z < 0.)
//}
//
//float pcf(float4 pos)
//{
//	float texsize = 1.0f / 1024;
//	float3 projpos = pos.xyz / pos.w;
//	float3 uv = projpos * 0.5 + 0.5;
//	
//	float factor = 0;
//	for (int x = -1; x <= 1; ++x)
//	{
//		for (int y = -1; y <= 1; ++y)
//		{
//			float2 offset = float2(x * texsize, y * texsize);
//			factor += 
//		}
//	}
//}

float4 ps(PS_INPUT input) : SV_TARGET
{
	float4 pos = mul(input.worldPos, LightViewProjection);

	pos.x = (pos.x + 1) * 0.5;
	pos.y =  ( 1 - pos.y ) * 0.5;
	//float depth = max( lightmap1.Sample(sampLinear, pos.xy).r,  max(lightmap2.Sample(sampLinear, pos.xy).r, max( lightmap3.Sample(sampLinear, pos.xy).r, lightmap4.Sample(sampLinear, pos.xy).r)));
	float depth = lightmap1.Sample(sampLinear, pos.xy).r;
	//return float4( 1 - depth, 1 - depth, 1 - depth, 1);
	//return float4( 1 - pos.z,  1- pos.z,1- pos.z, 1);

	if (depth + 0.00001 < pos.z)
		return float4(0.3,0.3,0.3,1);
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

