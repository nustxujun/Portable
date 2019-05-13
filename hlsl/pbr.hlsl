Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);

SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);

cbuffer ConstantBuffer: register(b0)
{
	float4 LightDirection;
	float4 Color;
	float3 CameraPosition;
	matrix InvertViewProjection;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

static const float PI = 3.14159265359;

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}


float4 main(PS_INPUT input) : SV_TARGET
{
	float3 albedo = pow(albedoTexture.Sample(sampLinear, input.Tex).rgb, 2.2);
	const float3 metallic = 0.5;
	const float roughness = 0.5;

	float4 normalData = normalTexture.Sample(sampPoint, input.Tex);
	float3 N = 2.0f * normalData.xyz - 1.0f;

	float depthVal = depthTexture.Sample(sampPoint, input.Tex).r;
	float4 worldPos;
	worldPos.x = input.Tex.x * 2.0f - 1.0f;
	worldPos.y = -(input.Tex.y * 2.0f - 1.0f);
	worldPos.z = depthVal;
	worldPos.w = 1.0f;
	worldPos = mul(worldPos, InvertViewProjection);
	worldPos /= worldPos.w;


	float3 V = normalize(CameraPosition - worldPos.xyz);

	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

	float3 Lo = 0;

	// calculate per-light radiance
	float3 L = -normalize(LightDirection.xyz);
	float3 H = normalize(V + L);
	//float distance = length(lightPositions[i] - WorldPos);
	//float attenuation = 1.0 / (distance * distance);
	//float3 radiance = lightColors[i] * attenuation;
	float3 radiance = 3;

	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	float3 nominator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
	float3 specular = nominator / denominator;

	// add to outgoing radiance Lo
	float NdotL = max(dot(N, L), 0.0);
	Lo += (kD * albedo / PI + specular)  * NdotL * radiance;

	float3 ambient = 0.03 * albedo;// *ao;
	float3 color = ambient + Lo;

	color = color / (color + 1);
	color = pow(color, 1.0 / 2.2);

	return float4(color, 1.0);
}

