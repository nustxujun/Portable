


struct ConeTracingParams
{
	float3 start;
	float3 dir;
	Texture3D<float4> octree;
	SamplerState samp;
	int numMips;
	float theta;
	float range;
};
float4 coneTracing(ConeTracingParams params)
{
	float3 start = params.start;
	float3 dir = params.dir;
	float tantheta = tan(params.theta * 0.5);
	int numMips = params.numMips - 1;
	SamplerState samp = params.samp;
	Texture3D<float4> octree = params.octree;
	float range = params.range;
	float inv = 1 / range;

	start = start + dir * 1.414 ;
	float dist = 1;
	const float maxdist = range * 1.414;
	float alpha = 0;
	float3 color = 0;


	while (dist < maxdist && alpha < 1)
	{
		float diameter = max(1, 2 * tantheta * dist);
		float mip = log2(diameter);

		float3 tc = start + dir * dist;
		if (tc.x > range || tc.x < 0 || 
			tc.y > range || tc.y < 0 || 
			tc.z > range || tc.z < 0  )
			break; 

		if (mip >= numMips)
			break;

		float4 sam = octree.SampleLevel(samp, tc *inv, mip);
		float a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;
		dist += diameter;
	}
	return float4(color, alpha);


	//int numstep = 0;
	//int maxstep = numMips;

	//while (numstep < maxstep)
	//{
	//	float level = pow(2, numstep);
	//	float radius = level * 0.5f;
	//	float len = radius / tan(theta * 0.5);

	//	float3 samplepos = start + dir * len;
	//	if (samplepos.x > range || samplepos.x < 0 ||
	//		samplepos.y > range || samplepos.y < 0 ||
	//		samplepos.z > range || samplepos.z < 0)
	//		return 0;
	//	float4 color = octree.SampleLevel(samp,  samplepos * inv,numstep);
	//	if (color.a > 0)
	//		return color;

	//	numstep++;
	//}

	//return 0;
}

