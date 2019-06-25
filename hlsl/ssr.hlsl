#include "pbr.hlsl"


Texture2D colorTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D depthTex:register(t2);
Texture2D depthbackTex:register(t3);
Texture2D materialTex:register(t4);
Texture2D noiseTex:register(t5);
Texture2D hitTex:register(t6);

SamplerState linearClamp: register(s0);
SamplerState pointClamp : register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix view;
	matrix proj;
	matrix invertProj;
	float reflection;
	float raylength;
	float width;
	float height;
	float stepstride;
	float stridescale;
	float nearZ;
	float jitter;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

float toView(float d)
{
	float4 p = mul(float4(0, 0, d, 1), invertProj);
	return p.z / p.w;
}

float3 toView(float3 pos)
{
	float4 p = mul(float4(pos, 1), invertProj);
	return (p / p.w).xyz;
}

float2 toScreen(float4 pos)
{
	return float2((pos.x *0.5 + 0.5) * width, (0.5 - pos.y * 0.5) * height);
}

const static float MAX_STEPS = 1024 ;

float4 raytracing(PS_INPUT Input) : SV_TARGET
{
	float4 material = materialTex.SampleLevel(pointClamp, Input.Tex, 0);
	if (material.b * reflection == 0)
		return 0;

	float roughness = material.r;
	float3 normal = normalTex.SampleLevel(pointClamp, Input.Tex,0).xyz;
	normal = normalize(mul(normal, (float3x3)view));
	
	float depth = depthTex.SampleLevel(pointClamp, Input.Tex,0).r;
	float4 projpos;
	projpos.x = Input.Tex.x * 2.0f - 1.0f;
	projpos.y = -(Input.Tex.y * 2.0f - 1.0f);
	projpos.z = depth;
	projpos.w = 1.0f;

	float4 posInView = mul(projpos, invertProj);
	posInView /= posInView.w;


	float3 V = normalize(posInView.xyz);
	float3 R = normalize(reflect(V, normal));

	
	//float raylen = length(posInView.xyz) * raylength;
	float raylen = raylength * 1000;
	raylen = (posInView.z + R.z * raylen) < nearZ ? (nearZ - posInView.z) / R.z : raylen;

	float4 endpos = float4(posInView.xyz + R * raylen, 1);


	float4 op = mul(posInView, proj);
	float4 ep = mul(endpos, proj);

	float ko = 1.0f / op.w;
	float ke = 1.0f / ep.w;

	op *= ko;
	ep *= ke;

	float2 os = toScreen(op);
	float2 es = toScreen(ep);



	float3 ov = posInView.xyz * ko;
	float3 ev = endpos.xyz * ke;

	
	es += dot(es - os, es - os) < 0.00001f ? 0.01f : 0.0f;

	float2 ds = es - os;
	float ls = length(ds);
	float2 delta = ds / ls;


	float2 onepixelstep = 1.0f / abs(delta);
	float steplen = min(onepixelstep.x, onepixelstep.y);// make to sample one pixel at least
	float numsteps = ls / steplen;

	float scale = 1.0f - min(1.0f, posInView.z * stridescale);
	float stride = 1 + stepstride * scale;
	float step = stride;
	float4 pqk = float4(ov, ko);
	float4 dpqk = (float4(ev, ke) - pqk) / numsteps;


	//float jitter = stride > 1 ? float(int(Input.Pos.x + Input.Pos.y) & 1) * 0.5f : 0;
	//pqk += dpqk * jitter;


	float maxsteps = min(numsteps, MAX_STEPS * stride);
	while (step < maxsteps)
	{
		float stepsize = step * steplen ;
		float2 coord = Input.Pos.xy + delta * stepsize;
		if (coord.x > width || coord.x < 0 || coord.y > height || coord.y < 0)
			return 0;

		float4 samplepos = (pqk + dpqk * step);
		samplepos.xyz /= samplepos.w;

		uint3 intcoord = uint3(coord, 0);

		float fdepth = depthTex.Load(intcoord).r;
		float hitdepth = fdepth;
		fdepth = toView(fdepth);
		float bdepth = depthbackTex.Load(intcoord).r;
		bdepth = toView(bdepth);
		if (samplepos.z > fdepth  && samplepos.z < bdepth )
		{
			if (stride == 1)
			{
				coord /= float2(width, height);
				float mask = pow( 1 - max(2* step / numsteps - 1 ,0)  ,2);
				mask *= saturate(raylen - dot(samplepos.xyz - posInView.xyz, R));
				float3 hitnormal = normalTex.Load(intcoord).xyz;
				hitnormal = mul(hitnormal, (float3x3)view);
				if (dot(hitnormal, R) > 0)
					mask = 0;
				//return colorTex.Load(intcoord) * reflectable * ( 1 - (float)step / (float)numsteps) *  mask;
				return float4(coord, hitdepth, (1 - step / numsteps) * mask);
			}
			else
			{
				stride = max(1,stride * 0.5f);
				step = step - stride;
				continue;
			}
		}
		step += stride;
	}

	return 0;

}



float SSR_BRDF(float3 V, float3 L, float3 N, float Roughness)
{
	float3 H = normalize(L + V);


	float D = DistributionGGX(N,H,Roughness);
	float G = GeometrySmith(N,V,L, Roughness);

	return max(0, D * G);
}

float4 filter(PS_INPUT Input):SV_TARGET
{
	float3 normal = normalTex.SampleLevel(pointClamp, Input.Tex,0).xyz;
	normal = normalize(mul(normal, (float3x3)view));
	float depth = depthTex.SampleLevel(pointClamp, Input.Tex,0).r;
	float4 projpos;
	projpos.x = Input.Tex.x * 2.0f - 1.0f;
	projpos.y = -(Input.Tex.y * 2.0f - 1.0f);
	projpos.z = depth;
	projpos.w = 1.0f;

	float4 posInView = mul(projpos, invertProj);
	posInView /= posInView.w;

	float roughness = materialTex.SampleLevel(pointClamp, Input.Tex, 0).r;
	float4 color = 0;

	float numweight = 0;
	int size = 1;
	for (int x = -size ; x <= size ; ++x)
	{
		for (int y = -size; y <= size; ++y)
		{
			float4 hit = hitTex.SampleLevel(linearClamp, Input.Tex, 0, int2(x, y));
			float3 hitndc = float3(hit.x * 2 - 1, 1 - hit.y * 2, hit.z);
			float3 hitPosinVS = toView(hitndc);

			float3 V = normalize(-posInView.xyz);
			float3 L = normalize(hitPosinVS - posInView.xyz);
			float weight = SSR_BRDF(V,L, normal, roughness);

			color += colorTex.SampleLevel(linearClamp, hit.xy, 0) *  hit.w * weight;
			//color += dot(normal, L);
			numweight += weight;
		}
	}

	color /= numweight;
	return color;
}