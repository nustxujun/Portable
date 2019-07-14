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

static const float VarianceClipGamma = 1.0f;
static const float Exposure = 10;
static const float BlendWeightLowerBound = 0.03f;
static const float BlendWeightUpperBound = 0.12f;
static const float BlendWeightVelocityScale = 100.0f * 60.0f;

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



#define USE_TONEMAP

float Luminance(in float3 color)
{
#ifdef USE_TONEMAP
	return color.r;
#else
	return dot(color, float3(0.25f, 0.50f, 0.25f));
#endif
}

float3 ToneMap(float3 color)
{
#ifdef USE_MIXED_TONE_MAP
	float luma = Luminance(color);
	if (luma <= MIXED_TONE_MAP_LINEAR_UPPER_BOUND)
	{
		return color;
	}
	else
	{
		return color * (MIXED_TONE_MAP_LINEAR_UPPER_BOUND * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - luma) / (luma*(2 * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - 1 - luma));
	}
#else
	return color / (1 + Luminance(color));
#endif
}

float3 UnToneMap(float3 color)
{
#ifdef USE_MIXED_TONE_MAP
	float luma = Luminance(color);
	if (luma <= MIXED_TONE_MAP_LINEAR_UPPER_BOUND)
	{
		return color;
	}
	else
	{
		return color * (MIXED_TONE_MAP_LINEAR_UPPER_BOUND * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - (2 * MIXED_TONE_MAP_LINEAR_UPPER_BOUND - 1) * luma) / (luma*(1 - luma));
	}
#else
	return color / (1 - Luminance(color));
#endif
}

float Luma4(float3 Color)
{
#ifdef USE_TONEMAP
	return Color.r;
#else
	return (Color.g * 2.0) + (Color.r + Color.b);
#endif
}

// Optimized HDR weighting function.
float HdrWeight4(float3 Color, float Exposure)
{
	return rcp(Luma4(Color) * Exposure + 4.0);
}

float4 sample_color(Texture2D tex, float2 uv)
{
	float4 c = tex.SampleLevel(samp, uv, 0);
#ifdef USE_TONEMAP
	c.rgb = ToneMap(c.rgb);
#endif


	//c.rgb = RGB_YCoCg(c.rgb);
	return float4(c.rgb, c.a);
	//return c;
}

float3 resolve_color(float3 color)
{
	//color = YCoCg_RGB (color);
#ifdef USE_TONEMAP
	color = UnToneMap(color);
#endif
	return color;
}


void findminmax(float2 uv, out float3 cmin, out float3 cmax, out float3 cavg)
{
	uint N = 9;
	float TotalWeight = 0.0f;
	float3 sum = 0.0f;
	float3 m1 = 0.0f;
	float3 m2 = 0.0f;
	float3 neighborMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
	float3 neighborMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
	float3 neighborhood[9];
	float neighborhoodHdrWeight[9];
	float neighborhoodFinalWeight = 0.0f;

	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			int i = (y + 1) * 3 + x + 1;
			float2 sampleOffset = float2(x, y) * texSize;
			float2 sampleUV = uv + sampleOffset;
			sampleUV = saturate(sampleUV);

			float3 NeighborhoodSamp = sample_color(frameTex, sampleUV).rgb;
			NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);

			neighborhood[i] = NeighborhoodSamp;
			neighborhoodHdrWeight[i] = HdrWeight4(NeighborhoodSamp, Exposure);
			neighborMin = min(neighborMin, NeighborhoodSamp);
			neighborMax = max(neighborMax, NeighborhoodSamp);

			m1 += NeighborhoodSamp;
			m2 += NeighborhoodSamp * NeighborhoodSamp;
			TotalWeight += neighborhoodFinalWeight;
			sum += neighborhood[i] * neighborhoodFinalWeight;
		}
	}


	// Variance clip.
	float3 mu = m1 / N;
	float3 sigma = sqrt(abs(m2 / N - mu * mu));
	cmin = mu - VarianceClipGamma * sigma;
	cmax = mu + VarianceClipGamma * sigma;
	cavg = mu;
}

float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
#ifdef CLIP_TO_CENTER
	// note: only clips towards aabb center (but fast!)
	float3 p_clip = 0.5 * (aabbMax + aabbMin);
	float3 e_clip = 0.5 * (aabbMax - aabbMin);

	float3 v_clip = prevSample - p_clip;
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

	if (ma_unit > 1.0)
		return p_clip + v_clip / ma_unit;
	else
		return prevSample;// point inside aabb
#else
	float3 r = prevSample - avg;
	float3 rmax = aabbMax - avg.xyz;
	float3 rmin = aabbMin - avg.xyz;

	const float eps = 0.000001f;

	if (r.x > rmax.x + eps)
		r *= (rmax.x / r.x);
	if (r.y > rmax.y + eps)
		r *= (rmax.y / r.y);
	if (r.z > rmax.z + eps)
		r *= (rmax.z / r.z);

	if (r.x < rmin.x - eps)
		r *= (rmin.x / r.x);
	if (r.y < rmin.y - eps)
		r *= (rmin.y / r.y);
	if (r.z < rmin.z - eps)
		r *= (rmin.z / r.z);

	return avg + r;
#endif
}


float2 sample_vel(float2 uv)
{
	float minZ = 3.402823466e+38F;
	float2 offset = 0;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float depth = depthTex.SampleLevel(samp, uv, 0, int2(x, y)).r;
			if (depth < minZ)
			{
				minZ = depth;
				offset = float2(x, y);
			}
		}
	}
	float2 vel = motionvecTex.SampleLevel(samp, uv + texSize * offset, 0).rg;
	vel.y = -vel.y;
	return vel;
}

float4 blendHistory(PS_INPUT input):SV_TARGET
{
	float2 uv = input.Tex;
	float2 uv_uj = input.Tex - jitter;

	float3 color = sample_color(frameTex, uv).rgb;
	float2 vel = sample_vel( uv_uj);
	float3 history = sample_color(historyTex,uv - vel).rgb;

	float3 cmin, cmax, cavg;
	findminmax(uv, cmin, cmax, cavg);

	//prevColor = ClipAABB(neighborMin, neighborMax, prevColor, (neighborMin + neighborMax) / 2);
	history = ClipAABB(cmin, cmax, history, cavg);

	float weightCurr = lerp(BlendWeightLowerBound, BlendWeightUpperBound, saturate(length(vel) * BlendWeightVelocityScale));
	float weightPrev = 1.0f - weightCurr;

	float RcpWeight = rcp(weightCurr + weightPrev);


	history = clamp(history, cmin.rgb, cmax.rgb);

	color = (color * weightCurr + history * weightPrev) * RcpWeight;

	color = resolve_color(color);
	return float4(color, 1);
}