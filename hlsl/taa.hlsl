Texture2D frameTex:register(t0);
Texture2D historyTex:register(t1);
Texture2D motionvecTex:register(t2);
Texture2D depthTex:register(t3);

SamplerState samp: register(s0);

cbuffer Constants:register(c0)
{
	float2 jitter;
	float2 texSize;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

// https://software.intel.com/en-us/node/503873
float3 RGB_YCoCg(float3 c)
{
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2
	// Cg = -R/4 + G/2 - B/4
	return float3(
		c.x / 4.0 + c.y / 2.0 + c.z / 4.0,
		c.x / 2.0 - c.z / 2.0,
		-c.x / 4.0 + c.y / 2.0 - c.z / 4.0
		);
}

// https://software.intel.com/en-us/node/503873
float3 YCoCg_RGB(float3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return saturate(float3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
		));
}

float4 sample_color(Texture2D tex, float2 uv)
{
	return tex.SampleLevel(samp, uv, 0);
}


float4 blendHistory(PS_INPUT input):SV_TARGET
{
	float2 uv = input.Tex;
	float2 uv_uj = input.Tex - jitter;
	float minZ = 3.402823466e+38F;
	float2 offset = 0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float depth = depthTex.SampleLevel(samp, uv_uj, 0, int2(x, y)).r;
			if (depth < minZ)
			{
				minZ = depth;
				offset = float2(x, y);
			}
		}
	}

	float2 vel = motionvecTex.SampleLevel(samp, uv_uj + texSize * offset, 0).rg;
	vel.y = -vel.y;
	float3 color = frameTex.SampleLevel(samp, uv, 0).rgb;


	// min max
	float2 du = float2(texSize.x, 0.0);
	float2 dv = float2(0.0, texSize.y);

	float4 ctl = sample_color(frameTex, uv - dv - du);
	float4 ctc = sample_color(frameTex, uv - dv);
	float4 ctr = sample_color(frameTex, uv - dv + du);
	float4 cml = sample_color(frameTex, uv - du);
	float4 cmc = sample_color(frameTex, uv);
	float4 cmr = sample_color(frameTex, uv + du);
	float4 cbl = sample_color(frameTex, uv + dv - du);
	float4 cbc = sample_color(frameTex, uv + dv);
	float4 cbr = sample_color(frameTex, uv + dv + du);

	float4 cmin = min(ctl, min(ctc, min(ctr, min(cml, min(cmc, min(cmr, min(cbl, min(cbc, cbr))))))));
	float4 cmax = max(ctl, max(ctc, max(ctr, max(cml, max(cmc, max(cmr, max(cbl, max(cbc, cbr))))))));

	float3 history = historyTex.SampleLevel(samp, uv - vel, 0).rgb;
	history = clamp(history, cmin.rgb, cmax.rgb);



	color = lerp(color, history, 0.9f);

	return float4(color, 1);
}