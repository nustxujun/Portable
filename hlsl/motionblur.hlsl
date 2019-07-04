Texture2D frameTex: register(t0);
Texture2D depthTex: register(t1);



SamplerState sampPoint : register(s0);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	matrix lastView;
	matrix proj;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

float3 toWorld(float3 pos_ndc)
{
	float4 pos = mul(float4(pos_ndc, 1), invertViewProj);
	pos /= pos.w;
	return pos.xyz;
}

float3 toNDC(float2 uv, float depth)
{
	return float3(uv.x * 2 - 1, 1 - uv.y * 2, depth);
}

const static uint NUM_SAMPLES = 50;

float4 main(PS_INPUT Input) : SV_TARGET
{
	float2 uv = Input.Tex;
	float depth  = depthTex.SampleLevel(sampPoint, uv, 0).r;
	float2 pos_ss = uv;
	float3 pos_ws = toWorld(toNDC(uv, depth));
	float4 oldpos_ss = mul(mul(float4(pos_ws,1), lastView), proj);
	oldpos_ss /= oldpos_ss.w;
	oldpos_ss.xy = float2(oldpos_ss.x *0.5 + 0.5, 0.5 - oldpos_ss.y * 0.5);
	float3 color = frameTex.SampleLevel(sampPoint, uv, 0).rgb;



	float2 vel = ((oldpos_ss.xyz - pos_ss) / (NUM_SAMPLES)).xy;



	for (uint i = 0; i < NUM_SAMPLES; ++i)
	{
		float2 offset = vel * ((float)i / float(NUM_SAMPLES ));
		color += frameTex.SampleLevel(sampPoint, uv + offset, 0).rgb;

		uv += vel;
	}
	color /= (float)NUM_SAMPLES;

	return float4(color, 1);
}


