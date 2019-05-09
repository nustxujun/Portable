cbuffer ConstantBuffer: register(b0)
{
	int kernelSize;
	float radius;
	float2 scale;
	float3 kernel[64];
}

cbuffer Matrix:register(b1)
{
	matrix invertProjection;
	matrix projection;
}

Texture2D normalTex: register(t0);
Texture2D depthTex: register(t1);
Texture2D noiseTex: register(t2);

SamplerState linearWrap : register(s0);


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT input) : SV_TARGET
{
	float3 normal = normalTex.Sample(linearWrap, input.Tex).rgb;
	normal = 2.0f * normal - 1.0f;
	normal = normalize(normal);
	float depth = depthTex.Sample(linearWrap, input.Tex).r;
	float4 pos;
	pos.x = input.Tex.x * 2.0f - 1.0f;
	pos.y = -(input.Tex.y * 2.0f - 1.0f);
	pos.z = depth;
	pos.w = 1.0f;

	pos = mul(pos, invertProjection);
	pos /= pos.w;

	float3 noise = noiseTex.Sample(linearWrap, input.Tex * scale).rgb;

	float3 tangent = normalize(noise - noise * dot(noise, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; ++i)
	{

		float3 vec = mul(kernel[i], TBN); 
		float4 sample = float4(pos.xyz, 1);


		sample = mul(sample, projection);
		sample.xyz /= sample.w;

		// Convert from ndc space to texture space
		if (sample.x < -1.0f || sample.x > 1.0f || sample.y < -1.0f || sample.y > 1.0f) {
			continue;
		}

		sample.xy = sample.xy * 0.5f + 0.5f;
		sample.y = 1.0f - sample.y;

		float sampleDepth = depthTex.Sample(linearWrap, sample.xy).r;

		if (sampleDepth <= sample.z) {
			occlusion += 1.0f;
		}
	}

	float total = (float)kernelSize;
	occlusion = pow(1.0f - occlusion / total, 1);

	return float4(occlusion, occlusion, occlusion, 1.0f);
}


