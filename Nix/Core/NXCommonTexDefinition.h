#pragma once 

enum NXCommonRTEnum
{
    NXCommonRT_DepthZ,
    NXCommonRT_DepthZ_R32,
    NXCommonRT_Lighting0,
    NXCommonRT_Lighting1,
    NXCommonRT_Lighting2,

    NXCommonRT_SSSLighting,

    // ��Ļ�ռ���Ӱ
    NXCommonRT_ShadowTest,

    // ����G-Buffer�ṹ���£�
    // RT0:		CustomData				        R8G8B8A8_UNORM
    // RT1:		Normal, SSSProfileIndex			R32G32B32A32_FLOAT
    // RT2:		Albedo					        R10G10B10A2_UNORM
    // RT3:		Metallic, Roughness, AO, flags	R8G8B8A8_UNORM
    //      flags Ŀǰֻ�� ShadingModel
    NXCommonRT_GBuffer0,
    NXCommonRT_GBuffer1,
    NXCommonRT_GBuffer2,
    NXCommonRT_GBuffer3,
    //NXCommonRT_GBuffer4,

    NXCommonRT_PostProcessing,

    NXCommonRT_SIZE,
};