Texture2D colorTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D depthTex:register(t2);
Texture2D depthbackTex:register(t3);

SamplerState linearWrap: register(s0);
SamplerState pointWrap : register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix proj;
	matrix view;
	matrix invertProj;
	float raylength;
	float width;
	float height;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

float toView(float d)
{
	float4 p = mul(float4(0, 0, d, 1), invertProj);
	return p.z / p.w;
}

float2 toScreen(float4 pos)
{
	return float2((pos.x *0.5 + 0.5) * width, (0.5 - pos.y * 0.5) * height);
}

const static uint MAX_STEPS = 100;

float4 main(PS_INPUT Input) : SV_TARGET
{
	float3 normal = normalTex.Sample(linearWrap, input.Tex).xyz;
	normal = normalize(mul(normal, (float3x3)view));
	float depth = depthTex.Sample(linearWrap, input.Tex).r;
	float4 pos;
	pos.x = input.Tex.x * 2.0f - 1.0f;
	pos.y = -(input.Tex.y * 2.0f - 1.0f);
	pos.z = depth;
	pos.w = 1.0f;

	pos = mul(pos, invertProj);
	pos /= pos.w;


	float3 V = normalize( pos);
	float3 R = reflect(V, normal);
	
	float4 endpos = float4(pos.xyz + R * raylength, 1);

	float4 op = mul(pos, proj);
	float4 ep = mul(endpos, proj);

	float ko = 1.0f / op.w;
	float ke = 1.0f / ep.w;

	op *= ko;
	ep *= ke;

	float2 os = toScreen(op);
	float2 es = toScreen(ep);

	float3 ov = V * ko;
	float3 ev = endpos.xyz * ke;

	float ls = length(es - os);
	float2 delta /= ls;

	float steplen = min(1.0f / delta.x, 1.0f / delta.y);// make to sample one pixel at least
	float numsteps = ls / steplen;
	float maxsteps = min(numsteps, MAX_STEPS);

	float4 dvk = float4(ev - ov, ke - ko) / numsteps;

	uint step = 1;
	while (step++ < maxsteps)
	{
		float stepsize = step * steplen;
		float2 coord = Input.Pos.xy + delta * stepsize;
		if (coord.x > width || coord.x < 0 || coord.y > height || coord.y < 0)
			return 0;

		float samplepos = (ov + dvk.xyz * step);
		float k = ko + dvk.w * step;
		samplepos /= k;

		coord /= float2(width, height);

		float fdepth = depthTex.Sample(linearWrap, coord).r;
		fdepth = toView(fdepth);
		float bdepth = depthbackTex.Sample(linearWrap, coord).r;
		bdepth = toView(bdepth);
		if (samplepos.z >= fdepth && sampepos.z <= bdepth)
		{
			return colorTex.Sample(linearWrap, uv) * 0.5f;
		}
	}

	return 0;

}


