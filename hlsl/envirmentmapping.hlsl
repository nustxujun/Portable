Texture2D frameTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D materialTex:register(t2);
Texture2D depthTex:register(t3);
#if IBL

#elif
TextureCube envTex: register(t1);
#endif


cbuffer ConstantBuffer: register(b0)
{
	matrix invertViewProj;
	float3 campos;
	float envCubeScale;
	float3 envCubeSize;
	float3 envCubePos;
}

SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);


cbuffer ConstantBuffer: register(b0)
{
	float threshold;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};


float3 intersectBox(float3 pos, float3 reflectionVector, float3 cubeSize, float3 cubePos)
{
	float3 rbmax = (0.5f * (cubeSize - cubePos) - pos) / reflectionVector;
	float3 rbmin = (-0.5f * (cubeSize - cubePos) - pos) / reflectionVector;

	float3 rbminmax = float3(
		(reflectionVector.x > 0.0f) ? rbmax.x : rbmin.x,
		(reflectionVector.y > 0.0f) ? rbmax.y : rbmin.y,
		(reflectionVector.z > 0.0f) ? rbmax.z : rbmin.z);

	float correction = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
	return (pos + reflectionVector * correction);
}


float4 main(PS_INPUT Input) : SV_TARGET
{
	float2 uv = Input.Tex;
	float3 color = frameTex.SampleLevel(sampPoint, uv, 0);
	float3 N = normalize(normalTex.SampleLevel(sampPoint, uv, 0));
	
	float depth = depthTex.SampleLevel(sampPoint, uv, 0).r;
	float4 worldpos;
	worldpos.x = uv.x * 2.0f - 1.0f;
	worldpos.y = -(uv.y * 2.0f - 1.0f);
	worldpos.z = depth;
	worldpos.w = 1.0f;

	worldpos = mul(worldpos, invertViewProj);
	worldpos /= worldpos.w;

	float3 V = normalize(campos - worldpos);
	float3 R = normalize(reflect(-V, N));

	float3 color = 0;
#if IBL
	float3 lut = LUT(N, V, roughness, lutTexture, sampPoint);
	float3 prefiltered = prefilteredTexture.SampleLevel(sampPoint, R, roughness * (PREFILTERED_MIP_LEVEL -1)).rgb;
	float3 irradiance = irradianceTexture.SampleLevel(sampPoint, N, 0).rgb;
	Lo += indirectBRDF(irradiance, prefiltered, lut, roughness, metallic, F0, albedo, N, V);
#endif

#if CORRECTED
	float3 corrected = intersectBox(pos, R, envCubeSize * envCubeScale, envCubePos);
	R = normalize(corrected - envCubePos);
#endif
	color = envReflectionTexture.SampleLevel(sampPoint, R, 0).rgb;

	return float4(color, 1);
}


