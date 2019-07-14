
float ComputeSquaredDistanceToAABB(float3 Pos, float3 AABBCenter, float3 AABBHalfSize)
{
	float3 delta = max(0, abs(AABBCenter - Pos) - AABBHalfSize);
	return dot(delta, delta);
}

bool TestSphereVsAABB(float3 sphereCenter, float sphereRadius, float3 AABBCenter, float3 AABBHalfSize)
{
	float distSq = ComputeSquaredDistanceToAABB(sphereCenter, AABBCenter, AABBHalfSize);
	return distSq <= sphereRadius * sphereRadius;
}

void createAABBFromFrustum(in float4 front, in float4 back, in float near , in float far, out float3 center, out float3 halflen)
{
	float3 aabbmin = float3(min(back.x, front.x), min(back.w, front.w), near);
	float3 aabbmax = float3(max(back.z, front.z), max(back.y, front.y), far);

	center = (aabbmax + aabbmin) * 0.5f;
	halflen = (aabbmax - aabbmin) * 0.5f;
}

