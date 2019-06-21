Texture2D colorTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D depthTex:register(t2);
Texture2D depthbackTex:register(t3);

SamplerState linearWrap: register(s0);
SamplerState pointWrap : register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix view;
	matrix proj;
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

const static uint MAX_STEPS = 1000;

float4 main(PS_INPUT Input) : SV_TARGET
{
	float3 normal = normalTex.Sample(linearWrap, Input.Tex).xyz;
	normal = normalize(mul(normal, (float3x3)view));
	float depth = depthTex.Sample(linearWrap, Input.Tex).r;
	float4 projpos;
	projpos.x = Input.Tex.x * 2.0f - 1.0f;
	projpos.y = -(Input.Tex.y * 2.0f - 1.0f);
	projpos.z = depth;
	projpos.w = 1.0f;

	float4 viewpos = mul(projpos, invertProj);
	viewpos /= viewpos.w;


	float3 V = normalize(viewpos.xyz);
	float3 R = normalize(reflect(V, normal));
	
	float raylen = (viewpos.z + R.z * raylength) >= 0 ? raylength : viewpos.z;
	float4 endpos = float4(viewpos.xyz + R * raylen, 1);


	float4 op = mul(viewpos, proj);
	float4 ep = mul(endpos, proj);

	float ko = 1.0f / op.w;
	float ke = 1.0f / ep.w;

	op *= ko;
	ep *= ke;

	float2 os = toScreen(op);
	float2 es = toScreen(ep);



	float3 ov = viewpos.xyz * ko;
	float3 ev = endpos.xyz * ke;

	float2 ds = es - os;
	float ls = length(ds);
	float2 delta = ds / ls;

	float2 steponpixel = 1.0f / abs(delta);
	float steplen = min(steponpixel.x, steponpixel.y);// make to sample one pixel at least
	float numsteps = ls / steplen;
	float maxsteps = numsteps;

	float4 dvk = float4(ev - ov, ke - ko) / maxsteps;

	int step = 1;
	while (step < maxsteps)
	{
		float stepsize = step * steplen;
		float2 coord = Input.Pos.xy + delta * stepsize;
		if (coord.x > width || coord.x < 0 || coord.y > height || coord.y < 0)
			return 0;

		float3 samplepos = (ov + dvk.xyz * step);
		float k = ko + dvk.w * step;
		samplepos /= k;

		coord /= float2(width, height);

		float fdepth = depthTex.SampleLevel(linearWrap, coord,0).r;
		fdepth = toView(fdepth);
		float bdepth = depthbackTex.SampleLevel(linearWrap, coord,0).r;
		bdepth = toView(bdepth);
		if (samplepos.z > fdepth + 0.0001 && samplepos.z < bdepth - 0.0001)
		{
			return normalTex.SampleLevel(linearWrap, coord,0) ;
		}
		step++;
	}

	return 0;

}


