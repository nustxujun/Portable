Texture3D<float4> voxelTex:register(t0);
RWTexture3D<float4> output:register(u0);
SamplerState samp: register(s0);

cbuffer Constants:register(c0)
{
	float level;
	float srcRange;
}

[numthreads(1, 1, 1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float3 uv = float3(globalIdx) / srcRange;
	float4 color = voxelTex.SampleLevel(samp, uv, level);

	output[globalIdx] = color;
}