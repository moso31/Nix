Texture2D txRenderResult : register(t0);
Texture2DArray txShadowMapDepth : register(t1);
SamplerState ssPointClamp : register(s0);

cbuffer CBufferParams : register(b1)
{
	float4 RTSize;
	float4 LayerParam0;
}

#define CascadeShadowMap_EnableLayer LayerParam0.x
#define CascadeShadowMap_ZoomScale LayerParam0.y

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

float4 DrawCascadeShadowMap(float2 InputUV, float4 InputColor)
{
	float2 NormalizedUV = InputUV * RTSize.xy * RTSize.ww;
	float2 ImageUV = NormalizedUV * 4.0f;
	float2 ZoomUV = ImageUV / CascadeShadowMap_ZoomScale + (CascadeShadowMap_ZoomScale - 1.0f) / (2.0f * CascadeShadowMap_ZoomScale);

	float4 OutColor = InputColor;

	if (NormalizedUV.x < 0.25f && NormalizedUV.y < 0.25f)
	{
		float3 uvw = float3(ZoomUV, 0.0f);
		txShadowMapDepth.Sample(ssPointClamp, uvw).xyz;

		OutColor = txShadowMapDepth.Sample(ssPointClamp, uvw).xxxx;
	}
	else if (NormalizedUV.x < 0.5f && NormalizedUV.y < 0.25f)
	{
		float2 offset = float2(1.0f, 0.0f) / CascadeShadowMap_ZoomScale;
		float3 uvw = float3(ZoomUV - offset, 1.0f);
		txShadowMapDepth.Sample(ssPointClamp, uvw).xyz;

		OutColor = txShadowMapDepth.Sample(ssPointClamp, uvw).xxxx;
	}
	else if (NormalizedUV.x < 0.75f && NormalizedUV.y < 0.25f)
	{
		float2 offset = float2(2.0f, 0.0f) / CascadeShadowMap_ZoomScale;
		float3 uvw = float3(ZoomUV - offset, 2.0f);
		txShadowMapDepth.Sample(ssPointClamp, uvw).xyz;

		OutColor = txShadowMapDepth.Sample(ssPointClamp, uvw).xxxx;
	}

	float Opacity = 1.0f;
	return lerp(InputColor, OutColor, Opacity);
}

float4 PS(PS_INPUT input) : SV_Target
{
	float4 result = txRenderResult.Sample(ssPointClamp, input.tex);

	if (CascadeShadowMap_EnableLayer)
		result = DrawCascadeShadowMap(input.tex, result);

	return result;
}
