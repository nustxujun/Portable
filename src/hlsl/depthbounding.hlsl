
Texture2D depthTex: register(t0);
RWTexture2D<float2> result: register(u0);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
}


groupshared uint min;
groupshared uint max;


[numthreads(16, 16, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx: SV_GroupID)
{
	uint cond = localIdx.x + localIdx.y;
	if (cond == 0)
	{
		min = 0x7f7fffff;
		max = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	float depth = depthTex.Load(uint3(globalIdx.x, globalIdx.y, 0)).x;

	float4 pos = float4(0, 0, depth, 1);
	pos = mul(pos, invertProj);
	uint z = asuint(pos.z / pos.w);

	if (depth != 0)
	{
		InterlockedMin(min, z);
		InterlockedMax(max, z);
	}

	GroupMemoryBarrierWithGroupSync();

	float minz = asfloat(min);
	float maxz = asfloat(max);

	if (cond == 0)
	{
		result[groupIdx.xy] = float2(minz, maxz);
	}
}