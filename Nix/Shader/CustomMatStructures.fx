#pragma once

struct VS_INPUT
{
    float4 pos : POSITION;
    float3 norm : NORMAL;
    float2 tex : TEXCOORD;
    float3 tangent : TANGENT;
};

struct PS_INPUT
{
    float4 posSS : SV_POSITION;
    float4 posOS : POSITION0;
    float4 posWS : POSITION1;
    float4 posVS : POSITION2;
    float3 normWS : NORMAL;
    float2 tex : TEXCOORD;
    float3 tangentWS : TANGENT;
#if GPU_INSTANCING
	nointerpolation uint instanceID : TEXCOORD1;
#endif
};

struct PS_OUTPUT
{
    float4 GBufferA : SV_Target0;
    float4 GBufferB : SV_Target1;
    float4 GBufferC : SV_Target2;
    float4 GBufferD : SV_Target3;
};

struct NXGBufferParams
{
    float3 albedo;
    float metallic;
    float3 normal;
    float roughness;
    float ao;
    float3 _0;
};
