
Texture2D depthboundsTex: register(t0);
RWTexture2D<float4> result: register(u0);




[numthreads(1, 1,1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx: SV_GroupID)
{
	//float depth = depthboundsTex.Load(uint3(globalIdx.x, globalIdx.y, 0)).x;

	result[globalIdx.xy] = float4(1,1,1,1);
}