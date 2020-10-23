// Shadow map
static const float SHADOWMAP_SIZE = 2048.0f;
static const float SHADOWMAP_INVSIZE = 1.0f / SHADOWMAP_SIZE;

static const float2 offset3x3[9] =
{
	float2(-1, -1), float2(0, -1), float2(1, -1),
	float2(-1,  0), float2(0,  0), float2(1,  0),
	float2(-1, +1), float2(0, +1), float2(1, +1)
};

float ShadowMapFilter(SamplerComparisonState samp, Texture2D tex, float4 pos)
{
	// Complete projection by doing division by w.
	pos.xyz /= pos.w;

	const float invSize = SHADOWMAP_INVSIZE;
	float percentLit = 0.0f;

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		// 对ShadowMap进行3x3范围采样
		float offset = offset3x3[i] * SHADOWMAP_INVSIZE;
		// 采用的比较是less，即：如果shadowMap的depth < 当前渲染坐标的depth，就计入percentLit。
		percentLit += tex.SampleCmpLevelZero(samp, pos.xy + offset, pos.z).r;
	}

	return percentLit /= 9.0f;
}

cbuffer ConstantBufferPrimitive : register(b0)
{
	matrix m_world;
}

cbuffer ConstantBufferShadowMapTransform : register(b1)
{
	matrix m_view;
	matrix m_projection;
	matrix m_tex;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 posH : SV_POSITION;
	float3 normW : NORMAL0;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posH = mul(input.pos, m_world);
	output.posH = mul(output.posH, m_view);
	output.posH = mul(output.posH, m_projection);
	output.normW = mul(float4(input.norm, 1.0), m_world).xyz;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
}
