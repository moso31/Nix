Texture2D txSceneLayer0 : register(t0);
Texture2D txSceneLayer1 : register(t1);
Texture2D txSceneLayer2 : register(t2);
Texture2D txSceneLayer3 : register(t3);
Texture2D txSceneLayer4 : register(t4);
Texture2D txSceneLayer5 : register(t5);
Texture2D txSceneLayer6 : register(t6);
Texture2D txSceneLayer7 : register(t7);
Texture2D txSceneLayer8 : register(t8);
Texture2D txSceneLayer9 : register(t9);
Texture2D txSceneLayer10 : register(t10);

SamplerState ssPointClamp : register(s0);

cbuffer cbParams : register(b4)
{
	int m_depthLayer;	// Depth Peeling 层数
	float3 _0;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 posSS : SV_POSITION;
	float2 tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.posSS = input.pos;
	output.tex = input.tex;

	return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
	float4 SceneColorLayer[11];
	SceneColorLayer[0] = txSceneLayer0.Sample(ssPointClamp, input.tex);
	SceneColorLayer[1] = txSceneLayer1.Sample(ssPointClamp, input.tex);
	SceneColorLayer[2] = txSceneLayer2.Sample(ssPointClamp, input.tex);
	SceneColorLayer[3] = txSceneLayer3.Sample(ssPointClamp, input.tex);
	SceneColorLayer[4] = txSceneLayer4.Sample(ssPointClamp, input.tex);
	SceneColorLayer[5] = txSceneLayer5.Sample(ssPointClamp, input.tex);
	SceneColorLayer[6] = txSceneLayer6.Sample(ssPointClamp, input.tex);
	SceneColorLayer[7] = txSceneLayer7.Sample(ssPointClamp, input.tex);
	SceneColorLayer[8] = txSceneLayer8.Sample(ssPointClamp, input.tex);
	SceneColorLayer[9] = txSceneLayer9.Sample(ssPointClamp, input.tex);
	SceneColorLayer[10] = txSceneLayer10.Sample(ssPointClamp, input.tex);

	float4 SceneColor = SceneColorLayer[m_depthLayer - 1];
	for (int i = m_depthLayer - 2; i >= 0; i--)
	{
		SceneColor = lerp(SceneColor, SceneColorLayer[i], SceneColorLayer[i].w);
	}
	return SceneColor;
}
