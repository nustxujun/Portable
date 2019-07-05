#ifdef CAMERA_MOTION_BLUR

Texture2D frameTex: register(t0);
Texture2D depthTex: register(t1);
Texture2D motionvecTex: register(t2);


SamplerState sampPoint : register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	matrix lastView;
	matrix proj;
}


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
	//float2 mvec = motionvecTex.SampleLevel(sampPoint, uv, 0).rg;
	float2 pos_ss = uv;
	float3 pos_ws = toWorld(toNDC(uv, depth));
	float4 oldpos_ss = mul(mul(float4(pos_ws,1), lastView), proj);
	oldpos_ss /= oldpos_ss.w;
	oldpos_ss.xy = float2(oldpos_ss.x *0.5 + 0.5, 0.5 - oldpos_ss.y * 0.5);
	float3 color = frameTex.SampleLevel(sampPoint, uv, 0).rgb;



	float2 vel = ((oldpos_ss.xy - pos_ss) / (NUM_SAMPLES)).xy;



	for (uint i = 0; i < NUM_SAMPLES; ++i)
	{
		float2 offset = vel * ((float)i / float(NUM_SAMPLES ));
		color += frameTex.SampleLevel(sampPoint, uv + offset, 0).rgb;

		uv += vel;
	}
	color /= (float)NUM_SAMPLES;

	return float4(color, 1);
}

#else

SamplerState sampPoint : register(s0);

#define PI 3.14159265358f

float4 tex2D(Texture2D tex, float2 uv)
{
	return tex.SampleLevel(sampPoint, uv, 0);
}

float SAMPLE_DEPTH_TEXTURE(Texture2D tex, float2 uv)
{
	return tex2D(tex, uv).r;
}

float LinearizeDepth(float depth)
{
	float4 pos = mul(float4(0, 0, depth, 1), invertProj);
	return pos.z / pos.w;
}

float2 VMax(float2 v1, float2 v2)
{
	return dot(v1, v1) < dot(v2, v2) ? v2 : v1;
}

struct v2f_img
{
	float4 Pos : SV_POSITION;
	float2 uv: TEXCOORD0;
};

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;

	float2 _CameraMotionVectorsTexture_TexelSize;
	float2 _VelocityScale;
	float _MaxBlurRadius;
	float2 _VelocityTex_TexelSize;
	float2 _NeighborMaxTex_TexelSize;
	float2 _MainTex_TexelSize;
	int _LoopCount;
}

// Fragment shader: Velocity texture setup
float4 frag_VelocitySetup(v2f_img i) : SV_Target
{
	// Sample the motion vector.
	float2 v = tex2D(_CameraMotionVectorsTexture, i.uv).rg;

	// Apply the exposure time and convert to the pixel space.
	v *= (_VelocityScale * 0.5) * _CameraMotionVectorsTexture_TexelSize;

	// Clamp the vector with the maximum blur radius.
	v /= max(1, length(v) * _MaxBlurRadius);

	// Sample the depth of the pixel.
	float d = LinearizeDepth(SAMPLE_DEPTH_TEXTURE(depthTex, i.uv));

	// Pack into 10/10/10/2 format.
	return float4((v * _MaxBlurRadius + 1) / 2, d, 0);
}

// Returns true or false with a given interval.
bool Interval(float phase, float interval)
{
	return frac(phase / interval) > 0.499;
}

// Interleaved gradient function from Jimenez 2014 http://goo.gl/eomGso
float GradientNoise(float2 uv)
{
	uv = floor(uv  * _ScreenParams.xy);
	float f = dot(float2(0.06711056f, 0.00583715f), uv);
	return frac(52.9829189f * frac(f));
}

// Jitter function for tile lookup
float2 JitterTile(float2 uv)
{
	float rx, ry;
	sincos(GradientNoise(uv + float2(2, 0)) * PI * 2, ry, rx);
	return float2(rx, ry) * _NeighborMaxTex_TexelSize.xy / 4;
}

// Velocity sampling function
float3 SampleVelocity(float2 uv)
{
	float3 v = tex2D(_VelocityTex, float4(uv, 0, 0)).xyz;
	return float3((v.xy ) * _MaxBlurRadius, v.z);
}

// Reconstruction fragment shader
float4 frag_Reconstruction(v2f_img i) : SV_Target
{
	// Color sample at the center point
	const float4 c_p = tex2D(_MainTex, i.uv0);

	// Velocity/Depth sample at the center point
	const float3 vd_p = SampleVelocity(i.uv);
	const float l_v_p = max(length(vd_p.xy), 0.5);
	const float rcp_d_p = 1 / vd_p.z;

	// NeighborMax vector sample at the center point
	const float2 v_max = tex2D(_NeighborMaxTex, i.uv + JitterTile(i.uv)).xy;
	const float l_v_max = length(v_max);
	const float rcp_l_v_max = 1 / l_v_max;

	// Escape early if the NeighborMax vector is small enough.
	if (l_v_max < 2) return c_p;

	// Use V_p as a secondary sampling direction except when it's too small
	// compared to V_max. This vector is rescaled to be the length of V_max.
	const float2 v_alt = (l_v_p * 2 > l_v_max) ? vd_p.xy * (l_v_max / l_v_p) : v_max;

	// Determine the sample count.
	const float sc = floor(min(_LoopCount, l_v_max / 2));

	// Loop variables (starts from the outermost sample)
	const float dt = 1 / sc;
	const float t_offs = (GradientNoise(i.uv) - 0.5) * dt;
	float t = 1 - dt / 2;
	float count = 0;

	// Background velocity
	// This is used for tracking the maximum velocity in the background layer.
	float l_v_bg = max(l_v_p, 1);

	// Color accumlation
	float4 acc = 0;

	while (t > dt / 4)
	{
		// Sampling direction (switched per every two samples)
		const float2 v_s = Interval(count, 4) ? v_alt : v_max;

		// Sample position (inverted per every sample)
		const float t_s = (Interval(count, 2) ? -t : t) + t_offs;

		// Distance to the sample position
		const float l_t = l_v_max * abs(t_s);

		// UVs for the sample position
		const float2 uv0 = i.uv + v_s * t_s * _MainTex_TexelSize.xy;
		const float2 uv1 = i.uv + v_s * t_s * _VelocityTex_TexelSize.xy;

		// Color sample
		const float3 c = tex2Dlod(_MainTex, float4(uv0, 0, 0)).rgb;

		// Velocity/Depth sample
		const float3 vd = SampleVelocity(uv1);

		// Background/Foreground separation
		const float fg = saturate((vd_p.z - vd.z) * 20 * rcp_d_p);

		// Length of the velocity vector
		const float l_v = lerp(l_v_bg, length(vd.xy), fg);

		// Sample weight
		// (Distance test) * (Spreading out by motion) * (Triangular window)
		const float w = saturate(l_v - l_t) / l_v * (1.2 - t);

		// Color accumulation
		acc += float4(c, 1) * w;

		// Update the background velocity.
		l_v_bg = max(l_v_bg, l_v);

		// Advance to the next sample.
		t = Interval(count, 2) ? t - dt : t;
		count += 1;
	}

	// Add the center sample.
	acc += float4(c_p.rgb, 1) * (1.2 / (l_v_bg * sc * 2));

	return float4(acc.rgb / acc.a, c_p.a);
}

#endif