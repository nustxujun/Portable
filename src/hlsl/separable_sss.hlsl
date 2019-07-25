Texture2D colorTex:register(t0);
Texture2D depthTex:register(t1);

SamplerState sampPoint: register(s0);
SamplerState sampLinear: register(s1);


cbuffer KernelConst: register(c0)
{
	float4 kernel[25];
	int numSamples;
}

cbuffer Constants: register(c1)
{
	matrix invertProj;
	float2 texelsize;
	float distToProjWin;
	float width;
}


float4 sampleTex(Texture2D tex, float2 uv)
{
	return tex.SampleLevel(sampPoint, uv, 0);
}

float4 sampleLinear(Texture2D tex, float2 uv)
{
	return tex.SampleLevel(sampLinear, uv, 0);
}

float lieanrDepth(float depth)
{
	float4 pos = mul(float4(0, 0, depth, 1), invertProj);
	return pos.z / pos.w;
}


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT input) :SV_TARGET
{
#if HORIZONTAL
	float2 dir = float2(1,0 );
#else
	float2 dir = float2( 0,1) ;
#endif

	float2 texcoord = input.Tex;

	// Fetch color of current pixel:
	float4 colorM = sampleTex(colorTex, texcoord);

	// Fetch linear depth of current pixel:
	float depthM = lieanrDepth(sampleTex(depthTex, texcoord).r);

	// Calculate the sssWidth scale (1.0 for a unit plane sitting on the
	// projection window):
	float distanceToProjectionWindow = distToProjWin;
	float scale = distanceToProjectionWindow / depthM;

	// Calculate the final step to fetch the surrounding pixels:
	float2 finalStep = width * scale * dir;
	//finalStep *= SSSS_STREGTH_SOURCE; // Modulate it using the alpha channel.
	finalStep *= 1.0 / 3.0; // Divide by 3 as the kernels range from -3 to 3.

	// Accumulate the center sample:
	float4 colorBlurred = colorM;
	colorBlurred.rgb *= kernel[0].rgb;

	// Accumulate the other samples:
	for (int i = 1; i < numSamples; i++) {
		// Fetch color and depth for current sample:
		float2 offset = texcoord + kernel[i].a * finalStep;
		float4 color = sampleLinear(colorTex, offset);

		// If the difference in depth is huge, we lerp color back to "colorM":
		float depth = lieanrDepth(sampleLinear(depthTex, offset).r);
		float s = saturate(300.0f * distanceToProjectionWindow *
			width * abs(depthM - depth));
		color.rgb = lerp(color.rgb, colorM.rgb, s);

		// Accumulate:
		colorBlurred.rgb += kernel[i].rgb * color.rgb;
	}

	return colorBlurred;
}
