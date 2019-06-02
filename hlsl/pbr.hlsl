Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);
Buffer<float4> lights: register(t3);

#ifdef TILED
Buffer<uint> lightsIndex: register(t4);
#endif

#ifdef CLUSTERED
Buffer<uint> lightsIndices: register(t4);
Texture3D<uint2> clusters: register(t5);


#endif

SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	matrix View;
	float3 clustersize;
	float roughness;
	float metallic;
	float width;
	float height;
	int maxLightsPerTile;
	int tilePerline;
	float near;
	float far;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
#ifndef TILED
#ifndef CLUSTERED
	uint index:TEXCOORD0;
#endif
#endif
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


#define NUM_SLICES 16

uint viewToSlice(float z)
{
	//return (uint)floor(NUM_SLICES * log(z / near) / log(far / near));
	return NUM_SLICES * (z - near) / (far - near);
}



float4 main(PS_INPUT input) : SV_TARGET
{
	float2 texcoord = float2(input.Pos.x / width, input.Pos.y / height);
	float4 texcolor = albedoTexture.Sample(sampLinear, texcoord);
	float3 albedo = pow(texcolor.rgb, 2.2);

	float4 normalData = normalTexture.Sample(sampPoint, texcoord);
	normalData = mul(normalData, View);
	float3 N = normalize(normalData.xyz);
	float depthVal = depthTexture.Sample(sampPoint, texcoord).g;
	float4 viewPos;
	viewPos.x = texcoord.x * 2.0f - 1.0f;
	viewPos.y = -(texcoord.y * 2.0f - 1.0f);
	viewPos.z = depthVal;
	viewPos.w = 1.0f;
	viewPos = mul(viewPos, invertProj);
	viewPos /= viewPos.w;


	float3 V = normalize( - viewPos.xyz);
	float3 F0 = 0.04;
	F0 = lerp(F0, albedo, metallic);

	float3 Lo = 0;


#ifdef TILED

	int x = input.Pos.x;
	int y = input.Pos.y;
	x /= 16;
	y /= 16;

	int startoffset = (x + y * tilePerline) * (maxLightsPerTile + 1);
	uint num = lightsIndex[startoffset];
	for (uint i = 1; i <= num; ++i)
	{
		uint index = lightsIndex[startoffset + i];
		float4 lightpos = lights[index * 2];
		float3 lightcolor = lights[index * 2 + 1].rgb;

#elif CLUSTERED
	uint x = input.Pos.x / clustersize.x;
	uint y = input.Pos.y / clustersize.y;
	uint z = viewToSlice(viewPos.z);
	uint3 coord = uint3(x, y, z);

	uint2 cluster = clusters[coord];

	//float4 final = float4((float)x / 256.0f, (float)y / 256.0f, (float)z/ 16.0f,1);
	//float4 final = (float)cluster.y / (float)100;
	//return (float)cluster.y / (float)100;
	for (uint i = 0; i < cluster.y; ++i)
	{
		uint index = lightsIndices[cluster.x + i];
		float4 lightpos = lights[index * 2];
		float3 lightcolor = lights[index * 2 + 1].rgb;

#else

	float4 lightpos = lights[input.index * 2];
	float3 lightcolor = lights[input.index * 2 + 1].rgb;
#endif

#ifdef POINT
	float3 L = normalize(lightpos.xyz - viewPos.xyz);


	float distance = length(lightpos.xyz - viewPos.xyz);
	if (distance > lightpos.w)
		continue;


	float attenuation = 1.0 / (distance * distance);
	float3 radiance = lightcolor * attenuation;


	//float x = distance / lightpos.w;
	//// fake inverse squared falloff:
	//// -(1/k)*(1-(k+1)/(1+k*x^2))
	//// k=20: -(1/20)*(1 - 21/(1+20*x^2))
	//float fFalloff = -0.05 + 1.05 / (1 + 20 * x*x);
	//radiance *= fFalloff;

#endif
#ifdef DIR
	float3 L = -normalize(lightpos.xyz);
	float3 radiance = lightcolor;

#endif
#ifdef SPOT
	float3 L = -normalize(lightpos.xyz - viewPos);
	float3 radiance = lightcolor;

#endif
	float3 H = normalize(V + L);


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
#if  TILED 
	}
#endif
#if CLUSTERED
	}

	
#endif


	//float3 ambient = 0.01 * albedo;// *ao;
	//float3 color = ambient + Lo;


	float3 color = Lo;
	//color = color / (color + 1);
	//color = pow(color, 1.0 / 2.2);

	return float4(color, texcolor.a);
}

