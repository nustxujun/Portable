
Buffer<float4> lights:register(t0);

RWBuffer<uint> curIndex:register(u0);
RWBuffer<uint2> lightIndices:register(u1);


#define GROUP_LEN 8
#define LIGHT_STRIDE GROUP_LEN * GROUP_LEN

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{


}

