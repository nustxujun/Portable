Texture2D frameTex: register(t0);
SamplerState smp: register(s0);

cbuffer Constants:register(c0)
{
	float2 texelsize;
	float2 sampleScale;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

float4 sampleBox(PS_INPUT Input) : SV_TARGET
{
	float4 fAvg = 0.0f;
	float4 d = texelsize.xyxy * float4(1.0f, 1.0f, -1.0f, -1.0f) * sampleScale.xyxy;

	fAvg += frameTex.Sample(smp, Input.Tex + d.xy);
	fAvg += frameTex.Sample(smp, Input.Tex + d.xw);
	fAvg += frameTex.Sample(smp, Input.Tex + d.zy);
	fAvg += frameTex.Sample(smp, Input.Tex + d.zw);

	fAvg *= (1.0f / 4.0f);

	return fAvg;
}

