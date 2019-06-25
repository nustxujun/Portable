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

float2 toScreen(float4 pos)
{
	return float2((pos.x *0.5 + 0.5) * screenSize.x, (0.5 - pos.y * 0.5) * screenSize.y);
}

float3 mulTBN(float3 vec, float3 N)
{
	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize(cross(UpVector, N));
	float3 TangentY = cross(N, TangentX);
	return mul(vec, float3x3(TangentX, TangentY, N));
}

float4 ImportanceSampleGGX(float2 E, float Roughness) {
	float m = Roughness * Roughness;
	float m2 = max(m * m, MIN_ROUGHNESS);

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt((1 - E.y) / (1 + (m2 - 1) * E.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	float3 H = float3(SinTheta * cos(Phi), SinTheta * sin(Phi), CosTheta);

	float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
	float D = m2 / (PI * d * d);

	float PDF = D * CosTheta;
	return float4(H, PDF);
}


const static float MAX_STEPS = 1024 ;

bool trace(float3 origin, float3 dir, float jitter, out float3 hituv, out float numsteps, out float3 hitpoint)
{
	float raylen = (origin.z + dir.z * raylength) > nearZ ? ((nearZ - origin.z) / dir.z) : raylength;
	float3 endpoint = dir * raylen + origin;
	float4 H0 = mul(float4(origin, 1), proj);
	float4 H1 = mul(float4(endpoint, 1), proj);
	float k0 = 1 / H0.w;
	float k1 = 1 / H1.w;
	float2 P0 = H0.xy * k0;
	float2 P1 = H1.xy * k1;
	float3 Q0 = origin * k0;
	float3 Q1 = endpoint * k1;


	P1 = dot(P1 - P0, P1 - P0) < 0.0001 ? P0 + 0.01 : P1;

	float2 delta = P1 - P0;
	bool permute = false;
	if (abs(delta.x) < abs(delta.y))
	{
		permute = true;
		delta = delta.yx;
		P1 = P1.yx;
		P2 = P2.yx;
	}

	float stepdir = sign(delta.x);
	float invdx = stepdir / delta.x;
	float2 dP = half2(stepdir, invdx * delta.y);
	float3 dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;

	dP *= stepSize;
	dQ *= stepSize;
	dk *= stepSize;
	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	float3 Q = Q0;
	float k = k0;
	float preZMaxEstimate = origen.z;
	numsteps = 0;
	float rayZMax = preZMaxEstimate, rayZMin = prevZMaxEstimate;
	float end = P1.x * stepdir;
	float2 P = P0;

	bool stop = false;
	for (; (P.x * stepdir) <= end && numsteps < MAX_STEPS && !stop; P += dP, Q.z += dQ.z, k += dk, numsteps += 1)
	{
		rayZMin = prevZMaxEstimate;
		rayZMax = (dQ.z * 0.5 + Q.z) / (dk * 0.5 + k);
		prevZMaxEstimate = rayZMax;

		if (rayZMin > rayZMax) {
			swap(rayZMin, rayZMax);
		}

		hitPixel = permute ? P.yx : P;
		float fdepth = depthTex.Load(uint3(hitPixel.xy,0)).r;
		fdepth = toView(fdepth);
		float bdepth = depthBackTex.Load(uint3(hitPixel.xy, 0)).r;
		bdetph = toView(bdetph);

		bool intersecting = false;


		stop = traceBehind ? intersecting : isBehind;
	}

}



void raymarch(PS_INPUT Input, out float4 hitResult: SV_Target0, out float4 mask: SV_Target1) 
{
	hitResult = 0;
	mask = 0;
	float4 material = materialTex.SampleLevel(pointClamp, uv, 0);
	if (material.b * reflection == 0)
		return ;

	float roughness = material.r;
	float3 N = normalTex.SampleLevel(pointClamp, uv,0).xyz;
	N = normalize(mul(N, (float3x3)view));
	
	float depth = depthTex.SampleLevel(pointClamp, uv,0).r;
	
	float2 uv = Input.Tex;
	float3 ray_origin_vs = toView(float3(uv.x * 2.0f - 1.0f, 1,0f - uv.y * 2.0f, depth));
	float ray_bump = max(-0.01f * ray_origin_vs.z, 0.001f);
	float2 Xi = noiseTex.SampleLevel(pointClamp, float2(uv + jitter) * screenSize / noiseSize).rg;
	Xi.y = lerp(Xi.y, 0.0f, brdfBias);
	float jitterValue = Xi.x + Xi.y;

	float4 H = ImportanceSampleGGX(Xi, roughness);
	H.xyz = mulTBN(H.xyz, N);
	
	float3 ray_dir_vs = reflect(normalize(ray_origin_vs), H);

	if (ray_dir_vs.z > 0)
	{
		return ;
	}

	float3 hitpoint;
	float3 hituv;
	float numsteps;

	bool hit = trace(hitpoint,hituv, numsteps);
	float hitmask = 0;
	if (hit)
	{ 
		hitmask = pow(1 - max(2 * numsteps / MAX_STEPS - 1, 0), 2); //attenuate half part
		hitmask *= saturate(raylength - dot(hitpoint - ray_origin_vs, ray_dir_vs) )// attenuate the end
	
		float3 hitnormal = normalTex.SampleLevel(pointClamp, hituv, 0).xyz;
		hitnormal = normalize(mul(hitnormal, (float3x3)view));
		if (dot(hitnormal, ray_dir_vs) > 0)
			return ;
	}


	hitResult = float4(hituv, H.a);
	mask = hitmask;
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