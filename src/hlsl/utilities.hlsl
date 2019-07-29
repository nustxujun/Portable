
const static float PI = 3.14159265358f;

float3 getWorldPos(float2 uv, float depth, matrix invertViewProj)
{
	float4 pos = float4(
		uv.x * 0.5 + 0.5,
		0.5 - uv.y * 0.5,
		depth,
		1
	);

	pos = mul(pos, invertViewProj);
	return pos.xyz / pos.w;
}