Texture2D _MainTex:register(t0);

cbuffer ConstantBuffer: register(b0)
{
	float2 _MainTex_TexelSize;
	float2 _TileMaxOffs;
	float _MaxBlurRadius;
	int _TileMaxLoop;
}

struct v2f_img
{
	float4 Pos : SV_POSITION;
	float2 uv: TEXCOORD0;
};

SamplerState sampPoint : register(s0);

float4 tex2D(Texture2D tex, float2 uv)
{
	return tex.SampleLevel(sampPoint, uv, 0);
}

float2 VMax(float2 v1, float2 v2)
{
	return dot(v1, v1) < dot(v2, v2) ? v2 : v1;
}

// Fragment shader: TileMax filter (2 pixel width with normalization)
float4 frag_TileMax1(v2f_img i) : SV_Target
{
	float4 d = _MainTex_TexelSize.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

	float2 v1 = tex2D(_MainTex, i.uv + d.xy).rg;
	float2 v2 = tex2D(_MainTex, i.uv + d.zy).rg;
	float2 v3 = tex2D(_MainTex, i.uv + d.xw).rg;
	float2 v4 = tex2D(_MainTex, i.uv + d.zw).rg;

	v1 = (v1 * 2 - 1) * _MaxBlurRadius;
	v2 = (v2 * 2 - 1) * _MaxBlurRadius;
	v3 = (v3 * 2 - 1) * _MaxBlurRadius;
	v4 = (v4 * 2 - 1) * _MaxBlurRadius;

	return float4(VMax(VMax(VMax(v1, v2), v3), v4), 0, 0);
}

// Fragment shader: TileMax filter (2 pixel width)
float4 frag_TileMax2(v2f_img i) : SV_Target
{
	float4 d = _MainTex_TexelSize.xyxy * float4(-0.5, -0.5, 0.5, 0.5);

	float2 v1 = tex2D(_MainTex, i.uv + d.xy).rg;
	float2 v2 = tex2D(_MainTex, i.uv + d.zy).rg;
	float2 v3 = tex2D(_MainTex, i.uv + d.xw).rg;
	float2 v4 = tex2D(_MainTex, i.uv + d.zw).rg;

	return float4(VMax(VMax(VMax(v1, v2), v3), v4), 0, 0);
}

// Fragment shader: TileMax filter (variable width)
float4 frag_TileMaxV(v2f_img i) : SV_Target
{
	float2 uv0 = i.uv + _MainTex_TexelSize.xy * _TileMaxOffs.xy;

	float2 du = float2(_MainTex_TexelSize.x, 0);
	float2 dv = float2(0, _MainTex_TexelSize.y);

	float2 vo = 0;

	for (int ix = 0; ix < _TileMaxLoop; ix++)
	{
		for (int iy = 0; iy < _TileMaxLoop; iy++)
		{
			float2 uv = uv0 + du * ix + dv * iy;
			vo = VMax(vo, tex2D(_MainTex, uv).rg);
		}
	}

	return float4(vo, 0, 0);
}


float4 frag_NeighborMax(v2f_img i) : SV_Target
{
	const float cw = 1.01f; // center weight tweak

	float4 d = _MainTex_TexelSize.xyxy * float4(1, 1, -1, 0);

	float2 v1 = tex2D(_MainTex, i.uv - d.xy).rg;
	float2 v2 = tex2D(_MainTex, i.uv - d.wy).rg;
	float2 v3 = tex2D(_MainTex, i.uv - d.zy).rg;

	float2 v4 = tex2D(_MainTex, i.uv - d.xw).rg;
	float2 v5 = tex2D(_MainTex, i.uv).rg * cw;
	float2 v6 = tex2D(_MainTex, i.uv + d.xw).rg;

	float2 v7 = tex2D(_MainTex, i.uv + d.zy).rg;
	float2 v8 = tex2D(_MainTex, i.uv + d.wy).rg;
	float2 v9 = tex2D(_MainTex, i.uv + d.xy).rg;

	float2 va = VMax(v1, VMax(v2, v3));
	float2 vb = VMax(v4, VMax(v5, v6));
	float2 vc = VMax(v7, VMax(v8, v9));

	return float4(VMax(va, VMax(vb, vc)) / cw, 0, 0);
}
