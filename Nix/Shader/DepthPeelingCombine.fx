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
	float4 SceneColorLayer0 = txSceneLayer0.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer1 = txSceneLayer1.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer2 = txSceneLayer2.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer3 = txSceneLayer3.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer4 = txSceneLayer4.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer5 = txSceneLayer5.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer6 = txSceneLayer6.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer7 = txSceneLayer7.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer8 = txSceneLayer8.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer9 = txSceneLayer9.Sample(ssPointClamp, input.tex);
	float4 SceneColorLayer10 = txSceneLayer10.Sample(ssPointClamp, input.tex);

	float4 SceneColor = SceneColorLayer4;
	SceneColor = lerp(SceneColor, SceneColorLayer9, SceneColorLayer9.w);
	SceneColor = lerp(SceneColor, SceneColorLayer8, SceneColorLayer8.w);
	SceneColor = lerp(SceneColor, SceneColorLayer7, SceneColorLayer7.w);
	SceneColor = lerp(SceneColor, SceneColorLayer6, SceneColorLayer6.w);
	SceneColor = lerp(SceneColor, SceneColorLayer5, SceneColorLayer5.w);
	SceneColor = lerp(SceneColor, SceneColorLayer4, SceneColorLayer4.w);
	SceneColor = lerp(SceneColor, SceneColorLayer3, SceneColorLayer3.w);
	SceneColor = lerp(SceneColor, SceneColorLayer2, SceneColorLayer2.w);
	SceneColor = lerp(SceneColor, SceneColorLayer1, SceneColorLayer1.w);
	SceneColor = lerp(SceneColor, SceneColorLayer0, SceneColorLayer0.w);
	return SceneColor;
}
