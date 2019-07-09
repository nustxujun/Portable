#include "pbr.hlsl"


Texture2D colorTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D depthTex:register(t2);
Texture2D depthbackTex:register(t3);
Texture2D materialTex:register(t4);
Texture2D noiseTex:register(t5);
Texture2D hitTex:register(t6);
Texture2D envTex:register(t7);

SamplerState linearClamp: register(s0);
SamplerState pointClamp : register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix view;
	matrix proj;
	matrix invertProj;
	float2 jitter;
	float2 screenSize;
	float2 noiseSize;
	float reflection;
	float raylength;
	float stepstride;
	float stridescale;
	float nearZ;
	float brdfBias;
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

float3 toNDC(float2 pos_ss, float depth)
{
	pos_ss /= screenSize;
	pos_ss = float2(pos_ss.x * 2 - 1, 1 - pos_ss.y * 2);
	return float3(pos_ss, depth);
}


float3 toView(float2 pos_ss, float depth)
{

	return toView(toNDC(pos_ss, depth));
}



float2 toScreen(float4 pos)
{
	return float2((pos.x *0.5 + 0.5) * screenSize.x, (0.5 - pos.y * 0.5) * screenSize.y);
}




const static float MAX_STEPS = 1024 ;


bool raytrace(float3 origin, float3 dir, float jitter, out float3 hituv, out float numsteps, out float3 hitpoint)
{
	float raylen = (origin.z + dir.z * raylength) < nearZ ? ((nearZ - origin.z) / dir.z) : raylength;
	float3 endpoint = dir * raylen + origin;
	float4 H0 = mul(float4(origin, 1), proj);
	float4 H1 = mul(float4(endpoint, 1), proj);

	float k0 = 1 / H0.w;
	float k1 = 1 / H1.w;

	float2 P0 = toScreen(H0 * k0);
	float2 P1 = toScreen(H1 * k1);
	float3 Q0 = origin * k0;
	float3 Q1 = endpoint * k1;


	float yMax = screenSize.y - 0.5;
	float yMin = 0.5;
	float xMax = screenSize.x - 0.5;
	float xMin = 0.5;
	float alpha = 0;

	if (P1.y > yMax || P1.y < yMin)
	{
		float yClip = (P1.y > yMax) ? yMax : yMin;
		float yAlpha = (P1.y - yClip) / (P1.y - P0.y);
		alpha = yAlpha;
	}
	if (P1.x > xMax || P1.x < xMin)
	{
		float xClip = (P1.x > xMax) ? xMax : xMin;
		float xAlpha = (P1.x - xClip) / (P1.x - P0.x);
		alpha = max(alpha, xAlpha);
	}

	P1 = lerp(P1, P0, alpha);
	k1 = lerp(k1, k0, alpha);
	Q1 = lerp(Q1, Q0, alpha);

	P1 = dot(P1 - P0, P1 - P0) < 0.0001 ? P0 + 0.01 : P1;


	float2 delta = P1 - P0;
	float2 stepdir = normalize(delta);
	float steplen = min(abs(1 / stepdir.x), abs(1 / stepdir.y));
	delta /= steplen;
	float maxsteps = max(abs(delta.x), abs(delta.y));
	float invsteps = 1 / maxsteps * (1 + jitter);
	float2 dP = (P1 - P0)  * invsteps;
	float3 dQ = (Q1 - Q0) * invsteps;
	float dk = (k1 - k0) * invsteps;
	numsteps = 0;
	maxsteps = min(maxsteps, MAX_STEPS);
	hituv = 0;
	hitpoint = 0;
	float stride = stepstride;
	float2 invsize = 1.0f / screenSize;
	while (numsteps < maxsteps)
	{
		float curstep = numsteps + stride;
		hituv.xy = P0 + dP * curstep;
		hitpoint = (Q0 + dQ * curstep) / (k0 + dk * curstep);
		float depth = hitpoint.z;
		float fdepth = depthTex.SampleLevel(pointClamp, hituv.xy * invsize,0).r;
		fdepth = toView(fdepth);
		float bdepth = depthbackTex.SampleLevel(pointClamp, hituv.xy* invsize,0).r;
		bdepth = toView(bdepth);

		if (depth > fdepth )
		{
			if (depth < bdepth)
			{
				numsteps += stride;
				return true;
			}
			if (stride <= 1)
				numsteps = curstep;
			else
				stride = max(1, stride / 2);
		}
		else
		{
			numsteps = curstep;
			stride = min(stepstride, stride * 2);
		}
	}

	return false;
}


void raycast(PS_INPUT Input, out float4 hitResult: SV_Target0/*, out float4 mask: SV_Target1*/) 
{
	hitResult = 0;
	//mask = 0;
	float2 uv = Input.Tex;
	float4 material = materialTex.SampleLevel(pointClamp, uv, 0);
	if (material.b * reflection == 0)
		return ;

	float roughness = material.r;
	float3 N = normalTex.SampleLevel(pointClamp, uv,0).xyz;
	N = normalize(mul(N, (float3x3)view));
	
	float depth = depthTex.SampleLevel(pointClamp, uv,0).r;
	
	float3 ray_origin_vs = toView(float3(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth));

	float3 ray_dir_vs = normalize(reflect(normalize(ray_origin_vs), N));

	int2 noiseuv = uint2((uv ) * screenSize) % uint2(noiseSize);
	float2 Xi = noiseTex.Load(int3(noiseuv, 0)).rg;
	Xi *= jitter;
	//if (ray_dir_vs.z > 0)
	//{
	//	return ;
	//}

	float3 hitpoint = 0;
	float3 hituv = 0;
	float numsteps = 0;
	bool hit = raytrace(ray_origin_vs/* + N * ray_bump*/, ray_dir_vs, (Xi.x + Xi.y) * 0.5f,hituv, numsteps, hitpoint);
	float hitmask = 0;
	if (hit)
	{ 
		//hitmask = pow(1 - max(2 * numsteps / MAX_STEPS - 1, 0), 2); //attenuate half part
		//hitmask *= saturate(raylength - dot(hitpoint - ray_origin_vs, ray_dir_vs));// attenuate the end
		hitmask = pow (1 - numsteps / MAX_STEPS, 2);
	
		float3 hitnormal = normalTex.Load(int3(hituv.xy, 0)).xyz;
		hitnormal = normalize(mul(hitnormal, (float3x3)view));
		if (dot(hitnormal, ray_dir_vs) > 0)
			return ;

	}

	hitResult = float4(hituv.xy, hitmask, 0);
}



float isoscelesTriangleOpposite(float adjacentLength, float theta)
{
	// simple trig and algebra - soh, cah, toa - tan(theta) = opp/adj, opp = tan(theta) * adj, then multiply * 2.0f for isosceles triangle base
	return 2.0f *tan(theta)* adjacentLength;
}

float isoscelesTriangleInRadius(float a, float h)
{
	float a2 = a * a;
	float fh2 = 4.0f * h * h;
	return (a * (sqrt(a2 + fh2) - a)) / (4.0f * h);
}

float4 coneSampleWeightedColor(float2 samplePos, float mipChannel, float gloss)
{
	float3 sampleColor = colorTex.SampleLevel(linearClamp, samplePos, mipChannel).rgb;
	return float4(sampleColor * gloss, gloss);
}

float isoscelesTriangleNextAdjacent(float adjacentLength, float incircleRadius)
{
	// subtract the diameter of the incircle to get the adjacent side of the next level on the cone
	return adjacentLength - (incircleRadius * 2.0f);
}


float specularPowerToConeAngle(float specularPower)
{
	// based on phong distribution model
	//if (specularPower >= exp2(CNST_MAX_SPECULAR_EXP))
	//{
	//	return 0.0f;
	//}
	const float xi = 0.244f;
	float exponent = 1.0f / (specularPower + 1.0f);
	return acos(pow(xi, exponent));
}

float4 resolve(PS_INPUT Input) :SV_TARGET
{
	int3 index = int3(Input.Pos.xy, 0);
	float4 hit = hitTex.Load(index);
	float3 sampleColor = colorTex.Load(int3(hit.xy,0)).rgb;
	float4 material = materialTex.Load(int3(Input.Pos.xy, 0));
	float roughness = material.r;
	float metallic = material.g;
	float3 env = envTex.Load(int3(Input.Pos.xy, 0)).rgb;
	float k = hit.z ;
	return float4( max(sampleColor * k, env), 0);
}

//float4 conetrace(PS_INPUT Input) :SV_TARGET
//{
//	int3 index = int3(Input.Pos.xy, 0);
//	float4 hit = hitTex.Load(index);
//	if (hit.z == 0)
//		return 0;
//
//	float roughness = materialTex.Load(index).r;
//	float depth = depthTex.Load(index).r;
//	float2 pos_ss = Input.Tex;
//	float3 pos_vs = toView(Input.Pos.xy, depth);
//	
//	float3 V = normalize(pos_vs);
//	float3 N = normalTex.Load(index).xyz;
//	N = normalize(mul(N, (float3x3)view));
//
//
//	float NdotV = saturate(dot(N, -V));
//	float coneTheta = specularPowerToConeAngle(roughness) * 0.5f;
//
//	float2 deltaP = (hit.xy) / screenSize - pos_ss.xy;
//	float adjacentLength = length(deltaP);
//	float2 adjacentUnit = normalize(deltaP);
//
//	float4 totalColor = 0;
//	float remainingAlpha = 1;
//	float maxMipLevel = 4;
//
//
//	float gloss = 1.0f - roughness;
//	float glossMult = gloss;
//
//	//for (int i = 0; i < 14; ++i)
//	//{
//	//	float oppositeLength = isoscelesTriangleOpposite(adjacentLength, roughness);
//
//	//	float incircleSize = isoscelesTriangleInRadius(oppositeLength, adjacentLength);
//
//	//	float2 samplePos = pos_ss.xy + adjacentUnit * (adjacentLength - incircleSize);
//
//	//	float mipChannel = clamp(log2(incircleSize * max(screenSize.x, screenSize.y)), 0.0f, maxMipLevel);
//
//	//	float4 newColor = coneSampleWeightedColor(hit.xy / screenSize, mipChannel, glossMult);
//	//	return float4(newColor.rgb, 1);
//	//	//remainingAlpha -= newColor.a;
//	//	//if (remainingAlpha < 0.0f)
//	//	//{
//	//	//	newColor.rgb *= (1.0f - abs(remainingAlpha));
//	//	//}
//	//	//totalColor += newColor;
//
//	//	//if (totalColor.a >= 1.0f)
//	//	//{
//	//	//	break;
//	//	//}
//
//	//	//adjacentLength = isoscelesTriangleNextAdjacent(adjacentLength, incircleSize);
//	//	//glossMult *= gloss;
//	//}
//
//
//	//return float4(totalColor.rgb, 1);
//}
//
//
//
