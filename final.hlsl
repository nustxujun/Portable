Texture2D albedoTexture: register(t0);
Texture2D lightTexture: register(t1);


SamplerState sampLinear: register(s0);

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 diffuseColor = albedoTexture.Sample(sampLinear, input.Tex);
	float4 light = lightTexture.Sample(sampLinear, input.Tex);

	float3 diffuseLight = light.rgb;
	float specularLight = light.a;

	return float4((diffuseColor.rgb * diffuseLight + specularLight), diffuseColor.a);
	//return float4(diffuseColor,1);
}