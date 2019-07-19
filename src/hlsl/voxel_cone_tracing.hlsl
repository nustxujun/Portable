


struct ConeTracingParams
{
	float3 start;
	float3 dir;
	Texture3D<float4> octree;
	SamplerState samp;
	int numMips;
	float theta;
};
float4 coneTracing(ConeTracingParams params)
{
	return params.octree.SampleLevel(params.samp, params.start, 0);
	float3 start = params.start;
	float3 dir = params.dir;
	float theta = params.theta;
	int numMips = params.numMips;
	SamplerState samp = params.samp;
	Texture3D<float4> octree = params.octree;

	int numstep = 0;
	int maxstep = numMips;

	while (numstep < maxstep)
	{
		float level = pow(2, numstep);
		float radius = level * 0.5f;
		float len = radius / tan(theta * 0.5);

		float3 samplepos = start + dir * len;

		samplepos /= pow(2,maxstep - numstep - 1);

		float4 color = octree.SampleLevel(samp,  samplepos,numstep);
		if (color.a > 0)
			return color;

		numstep++;
	}

	return 0;
}

