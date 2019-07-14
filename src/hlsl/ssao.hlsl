cbuffer ConstantBuffer: register(b0)
{
	float4 kernel[64];
	float2 scale;
	int kernelSize;
}

cbuffer Matrix:register(b1)
{
	matrix invertProjection;
	matrix projection;
	matrix view;
	float radius;
	float intensity;
}

Texture2D normalTex: register(t0);
Texture2D depthTex: register(t1);
Texture2D noiseTex: register(t2);

SamplerState linearWrap : register(s0);
SamplerState pointWrap : register(s1);


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float4 main(PS_INPUT input) : SV_TARGET
{
	float3 normal = normalTex.Sample(linearWrap, input.Tex).xyz;
	normal = normalize( mul(normal, (float3x3)view) );
	float depth = depthTex.Sample(pointWrap, input.Tex).r;
	float4 pos;
	pos.x = input.Tex.x * 2.0f - 1.0f;
	pos.y = -(input.Tex.y * 2.0f - 1.0f);
	pos.z = depth;
	pos.w = 1.0f;

	pos = mul(pos, invertProjection);
	pos /= pos.w;

	float3 noise = noiseTex.Sample(pointWrap, input.Tex * scale).rgb;

	float3 tangent = normalize(noise - normal * dot(noise, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, normal);


	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; ++i)
	{

		float3 vec = mul(kernel[i], TBN); 
		float4 sample = float4(pos.xyz + vec* radius, 1);
		float linearDepth = sample.z;
		sample = mul(sample, projection);
		sample.xyz /= sample.w;

		// Convert from ndc space to texture space
		if (sample.x < -1.0f || sample.x > 1.0f || sample.y < -1.0f || sample.y > 1.0f) {
			continue;
		}

		float2 uv = sample.xy * 0.5f + 0.5f;
		uv.y = 1.0f - uv.y;

		float sampleDepth = depthTex.Sample(pointWrap, uv.xy).r;
		float4 samplepos = float4(sample.xy, sampleDepth, 1);
		samplepos = mul(samplepos, invertProjection);
		samplepos /= samplepos.w;

		if (samplepos.z + 1.0f<= linearDepth && samplepos.z + 50.0f > linearDepth) {
			occlusion += 1.0f;
		}
	}

	float total = (float)kernelSize;
	occlusion = pow( (1.0f - occlusion / total)  , intensity);

	return float4(occlusion, occlusion, occlusion, 1.0f);
}


