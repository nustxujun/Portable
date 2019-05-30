
Texture2D depthboundsTex: register(t0);
Buffer<float4> lights: register(t1);
RWBuffer<uint> lightsOutput: register(u0);


cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	int numLights;
	float texelwidth;
	float texelheight;
	int maxLightsPerTile;
	int tilePerline;
}


// GPU-friendly version of the squared distance calculation 
// for the Arvo box-sphere intersection test
float ComputeSquaredDistanceToAABB(float3 Pos, float3 AABBCenter, float3 AABBHalfSize)
{
	float3 delta = max(0, abs(AABBCenter - Pos) - AABBHalfSize);
	return dot(delta, delta);
}

// Arvo box-sphere intersection test
bool TestSphereVsAABB(float3 sphereCenter, float sphereRadius, float3 AABBCenter, float3 AABBHalfSize)
{
	float distSq = ComputeSquaredDistanceToAABB(sphereCenter, AABBCenter, AABBHalfSize);
	return distSq <= sphereRadius * sphereRadius;
}

void genAABB(in float3 frontlt, in float3 frontrb, in float3 backlt, in float3 backrb,  out float3 center, out float3 halflen)
{
	float3 aabbmin = float3(min(backlt.x, frontlt.x), min(backrb.y, frontrb.y), frontlt.z);
	float3 aabbmax = float3(max(backrb.x, frontrb.x), max(backlt.y, frontlt.y), backlt.z);

	center = (aabbmax + aabbmin) * 0.5f;
	halflen = (aabbmax - aabbmin) * 0.5f;
}

void genLTRB(in float radio,in float4 flt, in float4 frb, in float z,  out float3 lt, out float3 rb)
{
	lt = float3(radio * flt.x, radio * flt.y, z);
	rb = float3(radio * frb.x, radio * frb.y, z);
}

[numthreads(1, 1,1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx: SV_GroupID)
{
	float2 db = depthboundsTex.Load(uint3(globalIdx.x, globalIdx.y, 0)).rg;
	float depthmin = db.x;
	float depthmax = db.y;
	//float depthmid = (depthmin + depthmax) * 0.5f;

	float2 coord = float2(float(globalIdx.x) * texelwidth, float(globalIdx.y) * texelheight);

	float2 lt = float2((float)coord.x  * 2.0f - 1.0f, 1.0f - (float)coord.y * 2.0f);
	float2 rb = float2((float)(coord.x + 0.0625f) * 2.0f - 1.0f, 1.0f - (float)(coord.y + 0.0625f)* 2.0f);

	float4 flt = mul(float4(lt, 1, 1), invertProj);
	flt /= flt.w;
	float4 frb = mul(float4(rb, 1, 1), invertProj);
	frb /= frb.w;

	float3 backlt, backrb, frontlt, frontrb, midlt, midrb;
	genLTRB(depthmax / flt.z, flt, frb, depthmax, backlt, backrb);
	genLTRB(depthmin / flt.z, flt, frb, depthmin, frontlt, frontrb);
	//genLTRB(depthmid / flt.z, flt, frb, depthmid, midlt, midrb);

	//float radio = depthmax / flt.z;
	//float2 backlt = float2(radio * flt.x, radio * flt.y);
	//float2 backrb = float2(radio * frb.x, radio * frb.y);
	//radio = depthmin / flt.z;
	//float2 frontlt = float2(radio * flt.x, radio * flt.y);
	//float2 frontrb = float2(radio * frb.x, radio * frb.y);

	//float3 aabbmin = float3(min(backlt.x, frontlt.x), min(backrb.y, frontrb.y), depthmin);
	//float3 aabbmax = float3(max(backrb.x, frontrb.x), max(backlt.y, frontlt.y), depthmax);

	//float3 aabbcenter = (aabbmax + aabbmin) * 0.5f;
	//float3 aabbhalf = (aabbmax - aabbmin) * 0.5f;

	float3 aabbcenter, aabbhalf;
	genAABB(frontlt, frontrb, backlt, backrb, aabbcenter, aabbhalf );


	uint startOffset = (globalIdx.x + globalIdx.y * tilePerline) * (maxLightsPerTile + 1);
	int lightcount = 0;
	for (int i = 0; i < numLights; ++i)
	{
		float4 light = lights[i * 2];
		float3 pos = light.xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbcenter, aabbhalf) )
			continue;

		lightsOutput[startOffset + 1 + lightcount] = i;

		lightcount++;
		if (lightcount >= maxLightsPerTile)
			break;
	}
	lightsOutput[startOffset] = lightcount;
}