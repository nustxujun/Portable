struct DirShadowMapParams
{
	matrix lightview;
	matrix lightProjs[8];
	float3 worldpos;
	int startcascade;
	int numcascades;
	float depthbias;
	float depth_viewspace;
	Texture2D shadowmap;
	SamplerComparisonState samp;
	float cascadedepths[8];
};


float getDirShadow(DirShadowMapParams params)
{
	int numcascades = params.numcascades;
	float depthbias = params.depthbias;
	float3 worldpos = params.worldpos;
	matrix lightView = params.lightview;
	Texture2D shadowmapTex = params.shadowmap;
	SamplerComparisonState samp = params.samp;
	float z = params.depth_viewspace;
	float scale = 1.0f / (float)numcascades;
	int startcascade = params.startcascade;

	for (int i = startcascade; i < numcascades; ++i)
	{
		if (z > params.cascadedepths[i])
			continue;

		float4 pos = mul(float4(worldpos,1), lightView);
		pos = mul(pos, params.lightProjs[i]);
		pos /= pos.w;


		float cmpdepth = pos.z - depthbias;

		pos.x = (pos.x + 1) * 0.5;
		pos.y = (1 - pos.y) * 0.5;

		float2 uv = (pos.xy + float2(i, 0)) * float2(scale, 1);
		float percentlit = 0.0f;
		for (int x = -2; x <= 2; ++x)
		{
			for (int y = -2; y <= 2; ++y)
			{
				percentlit += shadowmapTex.SampleCmpLevelZero(samp, uv, cmpdepth, int2(x, y));
			}
		}
		return  percentlit * 0.04f ;
	}

	return 1;
}

struct PointShadowMapParams
{
	float3 worldpos;
	float farZ;
	float3 lightpos;
	float depthbias;
	float scale;
	TextureCube shadowmap;
	SamplerComparisonState samp;
};

float getPointShadow(PointShadowMapParams params)
{
	float3 worldpos = params.worldpos;
	float farZ = params.farZ;
	float3 lightpos = params.lightpos;
	float depthbias = params.depthbias;
	TextureCube shadowmapTex = params.shadowmap;
	SamplerComparisonState samp = params.samp;
	float scale = params.scale;

	float3 uv = normalize(worldpos.xyz - lightpos);

	float cmpdepth = length(worldpos.xyz - lightpos) - depthbias * farZ;
	float percentlit = 0;
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -2; y <= 2; ++y)
		{
			for (int z = -2; z <= 2; ++z)
			{
				percentlit += shadowmapTex.SampleCmpLevelZero(samp, uv + float3(x, y, z) * scale, cmpdepth);
			}
		}
	}
	return percentlit * 0.008f;
}