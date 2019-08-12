#include "utilities.hlsl"


Texture2D depthTex: register(t0);

#if DIR
Texture2D 
#elif POINT
TextureCube
#else
Texture2D
#endif
shadowmapTex: register(t1);
Texture2D normalTex:register(t2);
Texture2D noiseTex: register(t3);


cbuffer ConstantBuffer: register(b0)
{
	matrix invertView;
	matrix invertProj;
	matrix lightView;
	matrix lightProjs[8];
	float4 cascadeDepths[8];
	float3 lightdir;
	int numcascades;
	float2 noiseSize;
	float2 screenSize;

	float scale;
	float shadowcolor;
	float depthbias;
	float translucency;
	float thickness;
	float translucency_bias;
	float near;
	float depthscale;
}
SamplerState sampLinear: register(s0);
SamplerState sampPoint: register(s1);
SamplerComparisonState sampshadow : register(s2);


struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

static const float2 PoissonOffsets[64] = {
	float2(0.0617981, 0.07294159),
	float2(0.6470215, 0.7474022),
	float2(-0.5987766, -0.7512833),
	float2(-0.693034, 0.6913887),
	float2(0.6987045, -0.6843052),
	float2(-0.9402866, 0.04474335),
	float2(0.8934509, 0.07369385),
	float2(0.1592735, -0.9686295),
	float2(-0.05664673, 0.995282),
	float2(-0.1203411, -0.1301079),
	float2(0.1741608, -0.1682285),
	float2(-0.09369049, 0.3196758),
	float2(0.185363, 0.3213367),
	float2(-0.1493771, -0.3147511),
	float2(0.4452095, 0.2580113),
	float2(-0.1080467, -0.5329178),
	float2(0.1604507, 0.5460774),
	float2(-0.4037193, -0.2611179),
	float2(0.5947998, -0.2146744),
	float2(0.3276062, 0.9244621),
	float2(-0.6518704, -0.2503952),
	float2(-0.3580975, 0.2806469),
	float2(0.8587891, 0.4838005),
	float2(-0.1596546, -0.8791054),
	float2(-0.3096867, 0.5588146),
	float2(-0.5128918, 0.1448544),
	float2(0.8581337, -0.424046),
	float2(0.1562584, -0.5610626),
	float2(-0.7647934, 0.2709858),
	float2(-0.3090832, 0.9020988),
	float2(0.3935608, 0.4609676),
	float2(0.3929337, -0.5010948),
	float2(-0.8682281, -0.1990303),
	float2(-0.01973724, 0.6478714),
	float2(-0.3897587, -0.4665619),
	float2(-0.7416366, -0.4377831),
	float2(-0.5523247, 0.4272514),
	float2(-0.5325066, 0.8410385),
	float2(0.3085465, -0.7842533),
	float2(0.8400612, -0.200119),
	float2(0.6632416, 0.3067062),
	float2(-0.4462856, -0.04265022),
	float2(0.06892014, 0.812484),
	float2(0.5149567, -0.7502338),
	float2(0.6464897, -0.4666451),
	float2(-0.159861, 0.1038342),
	float2(0.6455986, 0.04419327),
	float2(-0.7445076, 0.5035095),
	float2(0.9430245, 0.3139912),
	float2(0.0349884, -0.7968109),
	float2(-0.9517487, 0.2963554),
	float2(-0.7304786, -0.01006928),
	float2(-0.5862702, -0.5531025),
	float2(0.3029106, 0.09497032),
	float2(0.09025345, -0.3503742),
	float2(0.4356628, -0.0710125),
	float2(0.4112572, 0.7500054),
	float2(0.3401214, -0.3047142),
	float2(-0.2192158, -0.6911137),
	float2(-0.4676369, 0.6570358),
	float2(0.6295372, 0.5629555),
	float2(0.1253822, 0.9892166),
	float2(-0.1154335, 0.8248222),
	float2(-0.4230408, -0.7129914),
};

const static float invNumSamples = 1.0f / 64.0f;
const static float lightwidth = 0.01;

#if DIR



float4 receive_dir(float4 worldPos, float3 N, float depthlinear)
{

	for (int i = 0; i < numcascades; ++i)
	{
		if (depthlinear > cascadeDepths[i].r)
			continue;


		float4 pos = mul(worldPos, lightView);
		float depthinLightSpace = pos.z ;
		pos = mul(pos, lightProjs[i]);
		pos /= pos.w;


		pos.x = (pos.x + 1) * 0.5;
		pos.y = (1 - pos.y) * 0.5;

		float inv = 1.0f / (float)numcascades;
		float2 uv = (pos.xy + float2(i, 0)) * float2(inv, 1);


		depthinLightSpace = (depthinLightSpace - cascadeDepths[i].y) * cascadeDepths[i].z;

		//float searchwidth = lightwidth * pow(depthinLightSpace, 4);//saturate(depthinLightSpace - 0.5) / depthinLightSpace;

		//float sampledepth = shadowmapTex.Sample(sampLinear, uv).r;
		//float2 blocker = findblocker(uv, depthinLightSpace, searchwidth,jitter);
		//if (blocker.y < 1)
			//return 1;
		//float penumbra = (depthinLightSpace - blocker.x) / blocker.x * lightwidth;
		float receiver = exp(cascadeDepths[i].w * -depthinLightSpace);
		//return filter(uv, penumbra, receiver, jitter);
		return saturate(shadowmapTex.Sample(sampLinear, uv).r * receiver);

		float percentlit = 0.0f;
		//for (int x = -2; x <= 2; ++x)
		//{
		//	for (int y = -2; y <= 2; ++y)
		//	{
		//		percentlit += shadowmapTex.SampleCmpLevelZero(sampshadow, uv, cmpdepth, int2(x, y));
		//	}
		//}
#if TRANSMITTANCE
		float4 shrinkpos = float4(worldPos.xyz - N * translucency_bias, 1.0);
		shrinkpos = mul(shrinkpos, lightView);
		shrinkpos = mul(shrinkpos, lightProjs[i]);
		shrinkpos /= pos.w;
		shrinkpos.x = shrinkpos.x * 0.5 + 0.5;
		shrinkpos.y = 0.5 - shrinkpos.y * 0.5;

		uv = (shrinkpos.xy + float2(i, 0)) * float2(inv, 1);

		float d1 = shadowmapTex.Sample(sampLinear, uv).r;
		float d2 = shrinkpos.z;
		float d = (1 - translucency) * abs(d1 - d2) * cascadeDepths[i].r * thickness;
		float dd = -d * d;
		float3 profile = float3(0.233, 0.455, 0.649) * exp(dd / 0.0064) +
			float3(0.1, 0.336, 0.344) * exp(dd / 0.0484) +
			float3(0.118, 0.198, 0.0)   * exp(dd / 0.187) +
			float3(0.113, 0.007, 0.007) * exp(dd / 0.567) +
			float3(0.358, 0.004, 0.0)   * exp(dd / 1.99) +
			float3(0.078, 0.0, 0.0)   * exp(dd / 7.41);
		return float4(profile * saturate(0.3 + dot(-N, -lightdir)), percentlit * 0.04f * (1 - shadowcolor) + shadowcolor);
#else
		return  percentlit * 0.04f * (1 - shadowcolor) + shadowcolor;
#endif
	}
#if TRANSMITTANCE
	return float4(0, 0, 0, 1);
#else
	return 1;
#endif
}
#elif POINT



float4 receive_point(float4 worldPos, float3 N, float jitter)
{
	float3 lightpos = lightdir;
	float3 uv = worldPos.xyz - lightpos;

	//float3 randdir = frac(uv);
	float3 randdir = float3(cos(jitter), sin(jitter), 0);
	uv = normalize(uv);

	float3 tangent = normalize(cross(uv, randdir));
	float3 bitangent = normalize(cross(uv, tangent));

	//const float radius = 0.4;

	float cmpdepth = length(worldPos.xyz - lightpos) -1 ;
	float d = abs(worldPos.xyz - lightpos).z;
	float searchradius = lightwidth * (d - cascadeDepths[0].x) / d;
	//searchradius = 1;
	float dist = 0;
	float count = 0;
	const int num_sample = 64;
	float percentlit = 0;

	for (int i = 0; i < num_sample; ++i)
	{
		float3 offset = tangent * PoissonOffsets[i].x + bitangent * PoissonOffsets[i].y;
		offset *= searchradius;
	
		float d = shadowmapTex.SampleLevel(sampPoint, uv + offset, 0).r;
		if (cmpdepth > d)
		{
			dist += d;
			count += 1;
		}
		//percentlit += shadowmapTex.SampleCmpLevelZero(sampshadow, uv + offset, cmpdepth);
	}

	//return percentlit / (float)num_sample;

	//return shadowmapTex.SampleCmpLevelZero(sampshadow, uv , cmpdepth);
	//return 1 - count / (float)num_sample;
	if (count == 0)
		return 1;
	else if (count == num_sample)
		return 0;

	dist /= count;
	float pcfradius = (cmpdepth - dist) / dist * lightwidth ;
	//pcfradius = 0.001;

	for (int i = 0; i < num_sample; ++i)
	{
		float3 offset = tangent * PoissonOffsets[i].x + bitangent * PoissonOffsets[i].y;
		offset *= pcfradius;

		float depth = shadowmapTex.SampleLevel(sampPoint, uv + offset, 0);
		//if (depth> cmpdepth)

		percentlit += shadowmapTex.SampleCmpLevelZero(sampshadow, uv + offset, cmpdepth);
	}

	return percentlit / (float)num_sample;

	//const float len = 2;
	//for (int x = -len; x <= len; ++x)
	//{
	//	for (int y = -len; y <= len; ++y)
	//	{
	//		for (int z = -len; z <= len; ++z)
	//		{
	//			percentlit += shadowmapTex.SampleCmpLevelZero(sampshadow, uv + float3(x,y,z) * pcfradius, cmpdepth);
	//		}
	//	}
	//}
	//return percentlit / (pow(len * 2 + 1, 3));


	//float depthinLightSpace = length(worldPos.xyz - lightpos) * cascadeDepths[0].x ;

	//float sampledepth = shadowmapTex.SampleLevel(sampLinear, uv, 0).r;
	//float receiver = exp(cascadeDepths[0].y * -depthinLightSpace);
	//return saturate(sampledepth * receiver);




	//return percentlit * 0.008f;
}
#endif

float4 ps(PS_INPUT input) : SV_TARGET
{
	float3 N = normalTex.SampleLevel(sampPoint, input.Tex,0).rgb;
	N = normalize(N);
	float depth = depthTex.Sample(sampPoint, input.Tex).r;
	float4 worldPos = 0;
	worldPos.x = input.Tex.x * 2.0f - 1.0f;
	worldPos.y = -(input.Tex.y * 2.0f - 1.0f);
	worldPos.z = depth;
	worldPos.w = 1.0f;
	worldPos = mul(worldPos, invertProj);
	worldPos /= worldPos.w;
	float depthlinear = worldPos.z;
	worldPos = mul(worldPos, invertView);


#if DIR
	return receive_dir(worldPos, N, depthlinear);
#elif POINT
	int2 noiseuv = uint2((input.Tex)* screenSize) % uint2(noiseSize);
	float2 Xi = noiseTex.Load(int3(noiseuv, 0)).rg;
	float angle = (Xi.x + Xi.y) * 3.1415926;

	return receive_point(worldPos, N, angle);
#else
	return 1;
#endif
}


