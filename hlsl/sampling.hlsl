Texture2D frameTex: register(t0);
SamplerState smp: register(s0);
struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 downSampleBox(PS_INPUT Input) : SV_TARGET
{
	float4 fAvg = 0.0f;

	fAvg += frameTex.Sample(smp, Input.Tex, uint2(1, 1));
	fAvg += frameTex.Sample(smp, Input.Tex, uint2(1, -1));
	fAvg += frameTex.Sample(smp, Input.Tex, uint2(-1, 1));
	fAvg += frameTex.Sample(smp, Input.Tex, uint2(-1, -1));

	fAvg *= (1.0f / 4.0f);

	return fAvg;
}

//float4 upSampleBox(PS_INPUT Input) : SV_TARGET
//{
//	float4 fAvg = 0.0f;
//
//	fAvg += frameTex.Sample(smp, Input.Tex, uint2(1, 1));
//	fAvg += frameTex.Sample(smp, Input.Tex, uint2(1, -1));
//	fAvg += frameTex.Sample(smp, Input.Tex, uint2(-1, 1));
//	fAvg += frameTex.Sample(smp, Input.Tex, uint2(-1, -1));
//
//	fAvg *= (1.0f / 4.0f);
//
//	return fAvg;
//}