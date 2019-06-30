#include "ssr"

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

void raycast(PS_INPUT Input, out float4 hitResult: SV_Target0/*, out float4 mask: SV_Target1*/)
{
	hitResult = 0;
	//mask = 0;
	float2 uv = Input.Tex;
	float4 material = materialTex.SampleLevel(pointClamp, uv, 0);
	if (material.b * reflection == 0)
		return;

	float roughness = material.r;
	float3 N = normalTex.SampleLevel(pointClamp, uv, 0).xyz;
	N = normalize(mul(N, (float3x3)view));

	float depth = depthTex.SampleLevel(pointClamp, uv, 0).r;

	float3 ray_origin_vs = toView(float3(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, depth));
	float ray_bump = max(-0.01f * ray_origin_vs.z, 0.001f);
	int2 noiseuv = uint2((uv + jitter) * screenSize) % uint2(noiseSize);
	float2 Xi = noiseTex.Load(int3(noiseuv, 0)).rg;

	Xi.y = lerp(Xi.y, 0.0f, brdfBias);

	float4 H = ImportanceSampleGGX(Xi, roughness);
	H.xyz = mulTBN(H.xyz, N);
	//H = float4(N, 0);

	float3 ray_dir_vs = normalize(reflect(normalize(ray_origin_vs), H.xyz));

	//if (ray_dir_vs.z > 0)
	//{
	//	return ;
	//}

	float3 hitpoint = 0;
	float3 hituv = 0;
	float numsteps = 0;
	float jitterValue = Xi.x + Xi.y;
	bool hit = raytrace(ray_origin_vs/* + N * ray_bump*/, ray_dir_vs, jitterValue, hituv, numsteps, hitpoint);
	float hitmask = 0;
	if (hit)
	{
		//hitmask = pow(1 - max(2 * numsteps / MAX_STEPS - 1, 0), 2); //attenuate half part
		//hitmask *= saturate(raylength - dot(hitpoint - ray_origin_vs, ray_dir_vs));// attenuate the end
		hitmask = pow(1 - numsteps / MAX_STEPS, 2);

		float3 hitnormal = normalTex.Load(int3(hituv.xy, 0)).xyz;
		hitnormal = normalize(mul(hitnormal, (float3x3)view));
		if (dot(hitnormal, ray_dir_vs) > 0)
			return;

	}

	hitResult = float4(hituv.xy, hitmask, H.a);

}



float SSR_BRDF(float3 V, float3 L, float3 N, float Roughness)
{
	float3 H = normalize(L + V);


	float D = DistributionGGX(N, H, Roughness);
	float G = GeometrySmith(N, V, L, Roughness);

	return D * G;
}
static const float2 offset[4] = {
	float2(0, 0),
	float2(2, -2),
	float2(-2, -2),
	float2(0, 2)
};

float4 resolve(PS_INPUT Input) :SV_TARGET
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

	int2 noiseuv = int2((Input.Tex + jitter) * screenSize) % int2(noiseSize);
	float2 Xi = noiseTex.Load(int3(noiseuv, 0)).rg;
	float2x2 offsetRotMat = float2x2(Xi.x, Xi.y, -Xi.y, -Xi.x);

	float roughness = materialTex.SampleLevel(pointClamp, Input.Tex, 0).r;
	float4 color = 0;

	float numweight = 0;
	int size = 0;
	float3 V = normalize(-posInView.xyz);
	float NdotV = saturate(dot(normal, V));
	float coneTangent = lerp(0.0, roughness * (1.0 - brdfBias), NdotV * sqrt(roughness));

	for (int i = 0; i < 4; ++i)
	{
		float2 offsetuv = mul(offset[i], offsetRotMat) * (1 / screenSize);
		offsetuv += Input.Tex;
		float4 hit = hitTex.SampleLevel(pointClamp, offsetuv, 0);
		float2 uv = hit.xy / screenSize;
		float depth = depthTex.Load(uint3(hit.xy, 0)).r;
		float3 hitndc = float3(uv.x * 2 - 1, 1 - uv.y * 2, depth);
		float3 hitPosinVS = toView(hitndc);
		float3 L = normalize(hitPosinVS - posInView.xyz);

		float intersectionCircleRadius = coneTangent * length(uv.xy - Input.Tex.xy);
		float mip = clamp(log2(intersectionCircleRadius * max(screenSize.x, screenSize.y)), 0.0, 10);

		float weight = SSR_BRDF(V, L, normal, roughness) / max(1e-5, hit.a);
		//weight = 1.0;
		color.rgb += colorTex.SampleLevel(linearClamp, uv, mip).rgb * weight * hit.z;
		color.a += weight;
		//color += dot(normal, L);
		numweight += weight;
	}
	if (numweight == 0)
		return 0;
	color /= numweight;
	return color;
}
