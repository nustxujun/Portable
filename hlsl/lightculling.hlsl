
Texture2D depthboundsTex: register(t0);
Buffer lightslist: register(t1);
RWTexture2D<float4> result: register(u0);

cbuffer ConstantBuffer: register(b0)
{
	matrix invertProj;
	matrix view;
	float4 lights[100];
	float numLights;
}



[numthreads(1, 1,1)]
void main(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx: SV_GroupID)
{
	float2 db = depthboundsTex.Load(uint3(globalIdx.x, globalIdx.y, 0)).rg;
	float depthmin = db.x;
	float depthmax = db.y;

	float2 lt = float2((float)globalIdx.x  * 2.0f - 1.0f, 1.0f - (float)globalIdx.y * 2.0f);
	float2 rb = float2((float)(globalIdx.x + 1) * 2.0f - 1.0f, 1.0f - (float)(globalIdx.y + 1)* 2.0f);

	float4 flt = mul(float4(lt, 1, 1), invertProj);
	flt /= flt.w;

	float4 frb = mul(float4(rb, 1, 1), invertPorj);
	frb /= frb.w;

	float3 aabbmin = float3(flt.x, frb.y, depthmin);
	float3 aabbmax = float3(frb.x, flt.y, depthmax);

	for (int i = 0; i < numLights; ++i)
	{
		float4 light = lights[i];
		float4 pos = float4(light.xyz, 1);
		float range = light.w;


	}

	result[globalIdx.xy] = depth;


}