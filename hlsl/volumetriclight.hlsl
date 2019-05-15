Texture2D frameTex: register(t0);
SamplerState linearWrap : register(s0);

cbuffer ConstantBuffer: register(b0)
{
	float2 resolution;
	float2 lightpos;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT input) : SV_TARGET
{
	float NUM_SAMPLES = 500;
	float Density = 1;
	float Weight =0.1;
	float Decay = 0.99;

	float2 texCoord = input.Tex;
	// Calculate vector from pixel to light source in screen space.
	float2 deltaTexCoord = normalize(lightpos);
	// Divide by number of samples and scale by control factor.
	deltaTexCoord *= 1.0f / NUM_SAMPLES * Density;
	// Store initial sample.
	float4 color = frameTex.Sample(linearWrap, texCoord);

	// Set up illumination decay factor.
	float illuminationDecay = 1.0f;
	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		// Step sample location along ray.
		texCoord -= deltaTexCoord;
		// Retrieve sample at new location.
		float4 sample = frameTex.Sample(linearWrap, texCoord);
		// Apply sample attenuation scale/decay factors.
		sample *= illuminationDecay * Weight;
		// Accumulate combined color.
		color += sample;
		// Update exponential decay factor.
		illuminationDecay *= Decay;
	}
	// Output final color with a further scale control factor.
	return float4(color.rgb, 1);
}


