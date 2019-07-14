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

float4 main(PS_INPUT input) : SV_TARGET
{
	// Get normal data from the normal map.
	 float4 normalData = normalTexture.Sample(sampPoint, input.Tex);

	 // Transform normal back to [-1, 1] range;
	 float3 normal = 2.0f * normalData.xyz - 1.0f;

	 // Get specularPower, and get it into [0, 255] range.
	 float specularPower = normalData.a * 255;

	 // Get specularIntensity from the colorMap.
	 float specularIntensity = albedoTexture.Sample(sampLinear, input.Tex).a;

	 // Read depth.
	 float depthVal = depthTexture.Sample(sampPoint, input.Tex).r;

	 // Compute screen space poition;
	 float4 position;
	 position.x = input.Tex.x * 2.0f - 1.0f;
	 position.y = -(input.Tex.y * 2.0f - 1.0f);
	 position.z = depthVal;
	 position.w = 1.0f;

	 // Transform to world space.
	 position = mul(position, InvertViewProjection);
	 position /= position.w;

	 // Surface to light vector.
	 float3 lightVector = -normalize(LightDirection.xyz);
	 //float3 lightVector = -normalize(float3(0.5f, -0.3f, 0.0f));
	 // Compute diffuse light.
	 float NdL = max(0.3, dot(normal, lightVector));
	 float3 diffuseLight = NdL * Color.rgb;

	 // Reflection vector.
	 float3 reflectionVector = normalize(reflect(lightVector, normal));

	 // Camera to surface vector.
	 float3 directionToCamera = normalize(CameraPosition.xyz - position.xyz);

	 // Compute specular light.
	 float specularLight = specularIntensity * pow(saturate(dot(reflectionVector, directionToCamera)), specularPower);

	 //return float4(diffuseLight.rgb, 1.0f);
	 return float4(diffuseLight.rgb, specularLight);
}

