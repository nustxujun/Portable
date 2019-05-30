Texture2D albedoTexture: register(t0);
Texture2D normalTexture: register(t1);
Texture2D depthTexture: register(t2);
Buffer<float4> lights: register(t3);

#ifdef TILED
Buffer<uint> lightsIndex: register(t4);
#endif

SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	matrix View;
	float roughness;
	float metallic;
	float width;
	float height;
	int maxLightsPerTile;
	int tilePerline;
}

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
#ifndef TILED
	uint index:TEXCOORD0;
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

//float3 decode(float2 enc)
//{
//	float4 nn = float4(enc,0,0) * float4(2, 2, 0, 0) + float4(-1, -1, 1, -1);
//	float l = dot(nn.xyz, -nn.xyw);
//	nn.z = l;
//	nn.xy *= sqrt(l);
//	return nn.xyz * 2 + float3(0, 0, -1);
//}
//

float4 main(PS_INPUT input) : SV_TARGET
{
	float2 texcoord = float2(input.Pos.x / width, input.Pos.y / height);
	float4 texcolor = albedoTexture.Sample(sampLinear, texcoord);
	float3 albedo = pow(texcolor.rgb, 2.2);

	float4 normalData = normalTexture.Sample(sampPoint, texcoord);
	normalData = mul(normalData, View);
	float3 N = normalize(normalData.xyz);
	float depthVal = depthTexture.Sample(sampPoint, texcoord).g;
	float4 worldPos;
	worldPos.x = texcoord.x * 2.0f - 1.0f;
	worldPos.y = -(texcoord.y * 2.0f - 1.0f);
	worldPos.z = depthVal;
	worldPos.w = 1.0f;
	worldPos = mul(worldPos, invertProj);
	worldPos /= worldPos.w;


	float3 V = normalize( - worldPos.xyz);
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
	//return (float)num / (float)maxLightsPerTile;
	for (uint i = 1; i<= num; ++i)
	{
		uint index = lightsIndex[startoffset + i];
		float4 lightpos = lights[index * 2];
		float3 lightcolor = lights[index * 2 + 1].rgb;
#else

	float4 lightpos = lights[input.index * 2];
	float3 lightcolor = lights[input.index * 2 + 1].rgb;
#endif

#ifdef POINT
		float3 L = normalize(lightpos.xyz - worldPos.xyz);


		float distance = length(lightpos.xyz - worldPos.xyz);

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
		float3 L = -normalize(lightpos.xyz - worldPos);
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
		Lo += (kD * albedo / PI + specular)  * NdotL * radiance ;
#ifdef TILED
	}
#endif

	//float3 ambient = 0.1 * albedo;// *ao;
	//float3 color = ambient + Lo;


	float3 color = Lo;
	//color = color / (color + 1);
	//color = pow(color, 1.0 / 2.2);

	return float4(color, texcolor.a);
}

