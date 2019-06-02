
Buffer<float4> lights:register(t0);

RWBuffer<uint> curIndex:register(u0);
RWBuffer<uint> lightIndices:register(u1);
RWTexture3D<uint2> clusters:register(u2);


cbuffer Constants: register(c0)
{
	matrix invertProj;
	int numLights;
	float texelwidth;
	float texelheight;
	float nearZ;
	float farZ;
	float invclusterwidth;
	float invclusterheight;
};



#define LIGHT_THREAD 1
#define MAX_LIGHTS_PER_CLUSTER 100
#define NUM_SLICES 16

float sliceZ(uint level)
{
	float k = (float)level * (1.0f / (float)NUM_SLICES);
	//return nearZ * pow(farZ / nearZ, k);

	return nearZ + (farZ - nearZ) * k;
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

void genAABB(in float3 frontlt, in float3 frontrb, in float3 backlt, in float3 backrb, out float3 center, out float3 halflen)
{
	float3 aabbmin = float3(min(backlt.x, frontlt.x), min(backrb.y, frontrb.y), frontlt.z);
	float3 aabbmax = float3(max(backrb.x, frontrb.x), max(backlt.y, frontlt.y), backlt.z);

	center = (aabbmax + aabbmin) * 0.5f;
	halflen = (aabbmax - aabbmin) * 0.5f;
}

void genLTRB(in float radio, in float4 flt, in float4 frb, in float z, out float3 lt, out float3 rb)
{
	lt = float3(radio * flt.x, radio * flt.y, z);
	rb = float3(radio * frb.x, radio * frb.y, z);
}

groupshared float3 aabbCenter;
groupshared float3 aabbHalf;
groupshared uint groupCurIndex;
groupshared uint grouplights[MAX_LIGHTS_PER_CLUSTER];

[numthreads(LIGHT_THREAD, 1, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	uint threadindex = localIdx.x + localIdx.y;
	if (threadindex == 0)
	{
		groupCurIndex = 0;
		float2 coord = float2(float(groupIdx.x) * texelwidth, float(groupIdx.y) * texelheight);
		float2 lt = float2((float)coord.x  * 2.0f - 1.0f, 1.0f - (float)coord.y * 2.0f);
		float2 rb = float2((float)(coord.x + texelwidth) * 2.0f - 1.0f, 1.0f - (float)(coord.y + texelheight)* 2.0f);

		float4 flt = mul(float4(lt, 1, 1), invertProj);
		flt /= flt.w;
		float4 frb = mul(float4(rb, 1, 1), invertProj);
		frb /= frb.w;

		float near = sliceZ(groupIdx.z);
		float far = sliceZ(groupIdx.z + 1);

		float3 backlt, backrb, frontlt, frontrb;
		genLTRB(far / flt.z, flt, frb, far, backlt, backrb);
		genLTRB(near / flt.z, flt, frb, near, frontlt, frontrb);

		genAABB(frontlt, frontrb, backlt, backrb, aabbCenter, aabbHalf);
	}

	GroupMemoryBarrierWithGroupSync();

	for (int i = 0; i < numLights; i += LIGHT_THREAD)
	{
		float4 light = lights[i * 2];
		float3 pos = light.xyz;
		float range = light.w;

		if (!TestSphereVsAABB(pos, range, aabbCenter, aabbHalf))
			continue;

		uint index;
		InterlockedAdd(groupCurIndex, 1, index);
		if (index < MAX_LIGHTS_PER_CLUSTER)
			grouplights[index] = i;
		else
			break;
	}

	if (threadindex != 0)
		return;

	groupCurIndex = min(groupCurIndex, MAX_LIGHTS_PER_CLUSTER);
	uint offset;
	InterlockedAdd(curIndex[0], groupCurIndex, offset);
	clusters[groupIdx] = uint2(offset, groupCurIndex);
	for (uint i = 0; i < groupCurIndex; ++i)
		lightIndices[offset + i] = grouplights[i];
}

