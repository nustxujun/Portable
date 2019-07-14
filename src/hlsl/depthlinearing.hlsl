Texture2D depthTex: register(t0);
SamplerState smp : register(s0);

cbuffer Constants:register(b0)
{
	matrix invertProj;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};



float4 main(PS_INPUT Input) : SV_TARGET
{
	float depth = depthTex.Sample(smp, Input.Tex).r;
	float4 cp = float4(0, 0, depth, 1);
	cp = mul(cp, invertProj);
	cp /= cp.w;
	return float4(cp.w, depth,0,0);

}


