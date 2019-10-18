cbuffer vertexBuffer : register(b0)
{
	float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
	float2 pos : POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = mul( float4(input.pos.xy, 0.f, 1.f), ProjectionMatrix);

	output.col = input.col;
	output.uv = input.uv;
	return output;
}


Texture2D texture0:register(t0);
sampler sampler0
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

BlendState blend
{
	BlendEnable[0] = TRUE;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	BlendOp = ADD;
	SrcBlendAlpha = INV_SRC_ALPHA;
	DestBlendAlpha = ZERO;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F;
};

RasterizerState rasterizer
{
	FillMode = Solid;
	CullMode = None;
	ScissorEnable = true;
	DepthClipEnable = false;
};


float4 ps(PS_INPUT input) : SV_Target
{
	float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
	return out_col;
}

technique11 
{
	pass 
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetPixelShader(CompileShader(ps_5_0, ps()));

		SetBlendState(blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(rasterizer);
	}
}

