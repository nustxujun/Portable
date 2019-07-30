
const static float PI = 3.14159265358f;

float3 getWorldPos(float2 uv, float depth, matrix invertViewProj)
{
	float4 pos = float4(
		uv.x * 2 - 1,
		1 - uv.y * 2,
		depth,
		1
	);

	pos = mul(pos, invertViewProj);
	return pos.xyz / pos.w;
}