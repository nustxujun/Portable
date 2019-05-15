Texture2D frameTex: register(t0);
Texture2D lumTex: register(t1);
static const float  LUM_WHITE = 1.5f;
SamplerState PointWrap : register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

static const float  MIDDLE_GRAY = 0.72f;

float4 main(PS_INPUT Input) : SV_TARGET
{
	//float4 vColor = 0;
	float4 vColor = frameTex.Sample(PointWrap, Input.Tex);
	float4 vLum = lumTex.Sample(PointWrap, float2(0,0));
	//float3 vBloom = s2.Sample(LinearSampler, Input.Tex);

	// Tone mapping
	vColor.rgb *= MIDDLE_GRAY / (vLum.r + 0.001f);
	vColor.rgb *= (1.0f + vColor / LUM_WHITE);
	vColor.rgb /= (1.0f + vColor);

	//vColor.rgb += 0.6f * vBloom;
	vColor.a = 1.0f;

	return vColor;
}


