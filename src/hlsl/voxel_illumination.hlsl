#include "pbr.hlsl"

Texture3D<uint> albedoTex:register(t0);
Texture3D<uint> normalTex:register(t1);
Texture3D<uint> materialTex:register(t2);

Buffer<float4> pointlights:register(t3);
Buffer<float4> dirlights:register(t4);

RWTexture3D<float4> targetTex:register(u0);

cbuffer Constant:register(c0)
{
	float3 offset;
	float scaling;
	int size;
	int numpoints;
	int numdirs;
};

SamplerState sampLinear: register(s0);



float3 getWorldPos(uint3 index)
{
	return (float3)index * scaling + offset;
}

float3 getPosInVoxelSpace(float3 wp)
{
	return (wp - offset) / scaling;
}


float4 uintTofloat4(uint val)
{
	return float4(float((val & 0x000000FF)),
		float((val & 0x0000FF00) >> 8U),
		float((val & 0x00FF0000) >> 16U),
		float((val & 0xFF000000) >> 24U)) / 255;
}

bool checkVoxelExist(float3 pos)
{
	if (pos.x < 0 || pos.x > size || pos.y < 0 || pos.y > size || pos.z < 0 || pos.z > size)
		return false;
	uint c = albedoTex.Load(int4(pos,0));
	return (c & 0xFF000000) != 0;
}


float rayMarch(float3 pos, float3 dir, float range)
{
	float3 end = pos + dir * range;
	float3 delta = end - pos;
	float len = length(delta);

	float3 inv = 1 / dir;
	float step = min(min(abs(inv.x), abs(inv.y)), abs(inv.z));
	


	float maxstep = min(size, len / step);
	float numstep = 1;
	float visibility = 1;

	while (numstep <= maxstep)
	{
		float3 hit = pos + dir * numstep * step;
		if (checkVoxelExist(hit))
		{
			return 0;
			//if (visibility != 1)
			//	return 0;
			//else
			//	visibility = pow(length(pos - hit) / range, 1);
		}

		numstep += 1;
	}
	return visibility;
}

float3 brdf(float roughness, float metallic, float3 albedo, float3 normal, float3 tolight)
{
	float3 f0 = lerp(F0_DEFAULT, albedo, metallic);
	// assume V == N
	float3 kS = FresnelSchlickRoughness(normal, normal, f0, roughness);

	return Lambert(kS, albedo, metallic);
}


static const int3 offsets[6] = {
	int3(1,0,0),
	int3(-1,0,0),
	int3(0,1,0),
	int3(0,-1,0),
	int3(0,0,1),
	int3(0,0,-1),
};


float4 filter(Texture3D<uint> tex, int3 uv)
{
	float4 v;
	for (int i = 0; i < 6; i++)
	{
		float4 c = uintTofloat4(tex.Load(int4(uv +offsets[i], 0)));
		v += c;
	}
	return v / 6;
}

[numthreads(1, 1, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	//float3 albedo = uintTofloat4(albedoTex.Load(float4(globalIdx, 0))).rgb;
	float4 albedo = filter(albedoTex, globalIdx);
	//{
	//	for (int i = 0; i < 6; i++)
	//	{
	//		float3 c = uintTofloat4(albedoTex.Load(int4(int3(globalIdx)+offsets[i], 0))).rgb;
	//		albedo += c;
	//	}
	//}

	//albedo /= 6;

	int exist = checkVoxelExist(globalIdx);
	//if (!checkVoxelExist(globalIdx))
	//{
	//	targetTex[globalIdx] = float4(albedo, 0);
	//	return;
	//}

	//float3 normal = uintTofloat4(normalTex.Load(float4(globalIdx,0))).rgb;
	//normal = normal * 2 - 1;
	//float3 material = uintTofloat4(materialTex.Load(float4(globalIdx, 0))).rgb;

	float3 normal = filter(normalTex, globalIdx).rgb;
	normal = normal * 2 - 1;
	float4 material = filter(materialTex, globalIdx);

	float roughness = material.r;
	float metallic = material.g;


	float3 color = 0;
	float3 pos = globalIdx;
	//pos += 0.5;
	//pos.y +=  0.4;
	{
		for (int i = 0; i < numpoints; i++)
		{
			float4 lightpos = pointlights[i * 2];
			float4 lightcolor = pointlights[i * 2 + 1];

			float3 pos_vs = getPosInVoxelSpace(lightpos.xyz);
			float len = length(pos - pos_vs);
			float3 V = normalize( pos_vs - pos);

			float range = lightpos.w / scaling;
			if (len > range)
				continue;

			float shadow = rayMarch(pos, V, min(len, range));

			color += shadow * lightcolor.rgb * brdf(roughness, metallic, albedo.rgb, normal, V);
		}
	}

	{
		// max size of voxels scene
		float len = size * 1.74;
		for (int i = 0; i < numdirs; ++i)
		{
			float4 lightdir = dirlights[i * 2];
			float4 lightcolor = dirlights[i * 2 + 1];

			float3 V = -normalize(lightdir.xyz);

			float shadow = rayMarch(pos, V, len);
			//color += lightcolor.rgb * albedo;
			color += shadow * lightcolor.rgb * brdf(roughness, metallic, albedo.rgb, normal, V);
		}
	}

	targetTex[globalIdx] = float4(color , albedo.a);
	//targetTex[globalIdx] = float4(albedo.rgb, albedo.a);

}