struct RayParams
{
	matrix proj;
	matrix invertProj;
	float3 origin;
	float3 dir;
	float raylength;
	float nearZ;
	float jitter;
	float2 screenSize;
	float MAX_STEPS;
	float stepstride;
	Texture2D depthTex;
#if TEST_BACK
	Texture2D depthbackTex;
#else
	float thickness;
#endif
	SamplerState samp;
};

float toView(float d, matrix invproj)
{
	float4 p = mul(float4(0, 0, d, 1), invproj);
	return p.z / p.w;
}

float2 toScreen(float4 pos, float2 screenSize)
{
	return float2((pos.x *0.5 + 0.5) * screenSize.x, (0.5 - pos.y * 0.5) * screenSize.y);
}

bool raymarch(RayParams params, out float3 hituv, out float numsteps, out float3 hitpoint)
{
	float3 origin = params.origin;
	float3 dir = params.dir;
	float raylength = params.raylength;
	float nearZ = params.nearZ;
	float jitter = params.jitter;
	float2 screenSize = params.screenSize;
	matrix proj = params.proj;
	matrix invertProj = params.invertProj;
	Texture2D depthTex = params.depthTex;
#if TEST_BACK
	Texture2D depthbackTex = params.depthbackTex;
#else
	float thickness = params.thickness;
#endif
	SamplerState samp = params.samp;
	float MAX_STEPS = params.MAX_STEPS;
	float stepstride = params.stepstride;

	float raylen = (origin.z + dir.z * raylength) < nearZ ? ((nearZ - origin.z) / dir.z) : raylength;
	float3 endpoint = dir * raylen + origin;
	float4 H0 = mul(float4(origin, 1), proj);
	float4 H1 = mul(float4(endpoint, 1), proj);

	float k0 = 1 / H0.w;
	float k1 = 1 / H1.w;

	float2 P0 = toScreen(H0 * k0, screenSize);
	float2 P1 = toScreen(H1 * k1, screenSize);
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
		float fdepth = depthTex.SampleLevel(samp, hituv.xy * invsize, 0).r;
		fdepth = toView(fdepth, invertProj);

		if (depth > fdepth)
		{
#if TEST_BACK
			float bdepth = depthbackTex.SampleLevel(samp, hituv.xy* invsize, 0).r;
			bdepth = toView(bdepth, invertProj);
			if (depth < bdepth)
#else		
			if (depth < fdepth + thickness)
#endif
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
