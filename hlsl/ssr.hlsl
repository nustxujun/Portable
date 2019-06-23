Texture2D colorTex: register(t0);
Texture2D normalTex: register(t1);
Texture2D depthTex:register(t2);
Texture2D depthbackTex:register(t3);
Texture2D materialTex:register(t4);

SamplerState linearClamp: register(s0);
SamplerState pointClamp : register(s1);

cbuffer ConstantBuffer: register(b0)
{
	matrix view;
	matrix proj;
	matrix invertProj;
	float reflection;
	float raylength;
	float width;
	float height;
	float stepstride;
	float stridescale;
	float nearZ;
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

const static float MAX_STEPS = 1024 ;

float4 main(PS_INPUT Input) : SV_TARGET
{
	float reflectable = materialTex.SampleLevel(pointClamp, Input.Tex, 0).b * reflection;
	if (reflectable == 0)
		return 0;

	float3 normal = normalTex.SampleLevel(pointClamp, Input.Tex,0).xyz;
	normal = normalize(mul(normal, (float3x3)view));
	float depth = depthTex.SampleLevel(pointClamp, Input.Tex,0).r;
	float4 projpos;
	projpos.x = Input.Tex.x * 2.0f - 1.0f;
	projpos.y = -(Input.Tex.y * 2.0f - 1.0f);
	projpos.z = depth;
	projpos.w = 1.0f;

	float4 posInView = mul(projpos, invertProj);
	posInView /= posInView.w;


	float3 V = normalize(posInView.xyz);
	float3 R = normalize(reflect(V, normal));

	
	//float raylen = length(posInView.xyz) * raylength;
	float raylen = raylength * 1000;
	raylen = (posInView.z + R.z * raylen) < nearZ ? (nearZ - posInView.z) / R.z : raylen;

	float4 endpos = float4(posInView.xyz + R * raylen, 1);


	float4 op = mul(posInView, proj);
	float4 ep = mul(endpos, proj);

	float ko = 1.0f / op.w;
	float ke = 1.0f / ep.w;

	op *= ko;
	ep *= ke;

	float2 os = toScreen(op);
	float2 es = toScreen(ep);



	float3 ov = posInView.xyz * ko;
	float3 ev = endpos.xyz * ke;

	
	es += dot(es - os, es - os) < 0.00001f ? 0.01f : 0.0f;

	float2 ds = es - os;
	float ls = length(ds);
	float2 delta = ds / ls;


	float2 onepixelstep = 1.0f / abs(delta);
	float steplen = min(onepixelstep.x, onepixelstep.y);// make to sample one pixel at least
	float numsteps = ls / steplen;
	float4 dvk = float4(ev - ov, ke - ko) / numsteps;

	//float scale = pow( max(0.0625,1- depth), depth) ;
	float scale = 1.0f - min(1.0f, posInView.z * stridescale);
	float stride = 1 + stepstride * scale;
	float step = stride;

	numsteps = min(numsteps, MAX_STEPS * stride);

	while (step < numsteps)
	{
		float stepsize = step * steplen ;
		float2 coord = Input.Pos.xy + delta * stepsize;
		if (coord.x > width || coord.x < 0 || coord.y > height || coord.y < 0)
			return 0;

		float3 samplepos = (ov + dvk.xyz * step);
		float k = ko + dvk.w * step;
		samplepos /= k;

		//coord /= float2(width, height);
		uint3 intcoord = uint3(coord, 0);

		float fdepth = depthTex.Load(intcoord).r;
		fdepth = toView(fdepth);
		float bdepth = depthbackTex.Load(intcoord).r;
		bdepth = toView(bdepth);
		if (samplepos.z > fdepth  && samplepos.z < bdepth )
		{
			if (stride == 1)
				return colorTex.Load(intcoord) * reflectable * ( 1 - (float)step / (float)numsteps)  ;
			else
			{
				stride = max(1,stride * 0.5f);
				step = step - stride;
				continue;
			}
		}
		step += stride;
	}

	return 0;

}


