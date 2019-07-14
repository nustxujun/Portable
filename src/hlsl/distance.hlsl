Texture2D depthTex:register(t0);


SamplerState linearClamp: register(s0);

cbuffer Constant:register(b0)
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
	float depth = depthTex.SampleLevel(linearClamp, Input.Tex, 0);
	float4 viewpos = float4(
		Input.Tex.x * 2 -1,
		1 - Input.Tex.y * 2,
		depth,
		1);

	viewpos = mul(viewpos, invertProj);
	float dist = length(viewpos.xyz / viewpos.w);
	return dist;
}