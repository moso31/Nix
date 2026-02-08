#pragma once 

enum NXCommonRTEnum
{
    NXCommonRT_None,
    NXCommonRT_DepthZ,
    NXCommonRT_DepthZ_R32,
    NXCommonRT_Lighting0,
    NXCommonRT_Lighting1,
    NXCommonRT_Lighting2,

    NXCommonRT_SSSLighting,

    // 屏幕空间阴影
    NXCommonRT_ShadowTest,

    // 现行G-Buffer结构如下：
    // RT0:		CustomData				        R8G8B8A8_UNORM
    // RT1:		Normal, SSSProfileIndex			R32G32B32A32_FLOAT
    // RT2:		Albedo					        R10G10B10A2_UNORM
    // RT3:		Metallic, Roughness, AO, flags	R8G8B8A8_UNORM
    //      flags 目前只有 ShadingModel
    NXCommonRT_GBuffer0,
    NXCommonRT_GBuffer1,
    NXCommonRT_GBuffer2,
    NXCommonRT_GBuffer3,
    //NXCommonRT_GBuffer4,

    NXCommonRT_PostProcessing,
    NXCommonRT_DebugLayer,

    NXCommonRT_SIZE,
};

enum NXCommonTexEnum
{
    NXCommonTex_White,
    NXCommonTex_Normal,
    NXCommonTex_Noise2DGray_64x64,
    NXCommonTex_SIZE,
};
