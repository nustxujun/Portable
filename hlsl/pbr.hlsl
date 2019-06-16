
static const float PI = 3.14159265359f;
static const float3 F0_DEFAULT = 0.04f;
static const float MIN_ROUGHNESS = 0.0001f;

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS);
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


float3 CookTorrance(float3 N, float3 V, float3 L, float roughness, float metallic, float3 f0, out float3 kS)
{
	float3 H = normalize(V + L);
	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	float3 F = fresnelSchlick(max(dot(H, V), 0.0), f0);
	// output
	kS = F;

	float3 nominator = D * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001f;
	float3 specular = nominator / denominator;
	return specular;
}

float3 Lambert(float3 kS, float3 albedo, float metallic)
{
	float3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;
	return kD * albedo / PI;
}

float3 directBRDF(float roughness, float metallic, float3 f0, float3 albedo, float3 normal, float3 tolight, float3 tocam)
{
	f0 = lerp(f0, albedo, metallic);
	float3 V = normalize(tocam);
	float3 L = normalize(tolight);
	float3 N = normal;

	float3 kS = 0;
	float3 specBRDF = CookTorrance(N, V, L, roughness, metallic, f0, kS);
	float3 diffBRDF = Lambert(kS, albedo, metallic);

	float NdotL = max(dot(N, L), 0.0f);
	return (specBRDF + diffBRDF) * NdotL;
}


float3 FresnelSchlickRoughness(float3 v, float3 n, float3 f0, float roughness)
{
	float NdotV = max(dot(v, n), 0.0f);
	float r1 = 1.0f - roughness;
	return f0 + (max(float3(r1, r1, r1), f0) - f0) * pow(1 - NdotV, 5.0f);
}

float3 LUT(float3 normal, float3 viewDir, float roughness, Texture2D lut, SamplerState smp)
{
	float NdotV = dot(normal, viewDir);
	NdotV = max(NdotV, 0.0f);
	float2 uv = float2(NdotV, roughness);
	return lut.Sample(smp, uv);
}

float3 indirectBRDF(float3 irradiance,float3 prefilter, float3 lut, float roughness, float metallic, float3 f0, float3 albedo, float3 normal, float3 tocam)
{
	roughness = max(roughness, MIN_ROUGHNESS);
	f0 = lerp(f0, albedo, metallic);

	float3 kS = FresnelSchlickRoughness(tocam, normal, f0, roughness);
	float3 kD = 1.0f - kS;
	kD *= (1.0f - metallic);

	float3 specular = prefilter * (kS * lut.x + lut.y);
	float3 diffuse = irradiance * albedo;

	float3 ambient = (kD * diffuse + specular);

	return ambient;
}

float attenuate(float distance, float range)
{
	float att = saturate(1.0f - (distance * distance / (range * range)));
	return att * att;
}