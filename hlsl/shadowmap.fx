
Texture2D lightmap: register(t0);
SamplerState sampLinear: register(s0);

cbuffer ConstantBuffer: register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	matrix lightView;
	matrix lightProjs[8];
	float4 cascadeDepths[8];
	int numcascades;
	float scale;
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
	float4 pos = 0;
	for (int i = 0; i < numcascades; ++i)
	{
		if (input.Pos.z > cascadeDepths[i].z)
			continue;



		pos = mul(mul(input.worldPos, lightView), lightProjs[i]);
		pos.x = (pos.x + 1) * 0.5;
		pos.y = (1 - pos.y) * 0.5;

		float2 uv = (pos.xy + float2(i, 0)) * float2(scale, 1);
		float depth = lightmap.Sample(sampLinear, uv).r;
		if (depth + 0.001 < pos.z)
		{
			if (i == 0)
				return float4(0.3, 0, 0, 1);
			else if (i == 1)
				return float4(0, 0.3, 0, 1);
			else if (i == 2)
				return float4(0, 0.0, 0.3, 1);
			else
				return 0.3;
		}
		else
		{
			if (i == 0)
				return float4(0.9, 0, 0, 1);
			else if (i == 1)
				return float4(0, 0.9, 0, 1);
			else if (i == 2)
				return float4(0, 0.0, 0.9, 1);
			else
				return 1;
		}
	}
	
	return float4(0.8, 0.8,0,1);
}


technique11 
{
	pass receive
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}

