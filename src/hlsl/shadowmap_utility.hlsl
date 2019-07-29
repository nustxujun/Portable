struct ShadowMapParams
{
	matrix lightview;
	matrix lightProjs[8];
	float cascadedepths[8];
	int numcascades;
	float depthbias;
	float3 worldpos;
	Texture2D shadowmap;
	SamplerComparisonState samp;
};


float getShadow(ShadowMapParams params)
{
	int numcascades = params.numcascades;
	float depthbias = params.depthbias;
	float3 worldpos = params.worldpos;
	matrix lightView = params.lightview;
	Texture2D shadowmapTex = params.shadowmap;
	SamplerComparisonState samp = params.samp;
	float z = worldpos.z;
	float scale = 1.0f / (float)numcascades;

	for (int i = 0; i < numcascades; ++i)
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