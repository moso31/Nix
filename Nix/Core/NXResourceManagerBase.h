#pragma once
#include "NXInstance.h"

enum NXCommonRTEnum
{
    NXCommonRT_DepthZ,
    NXCommonRT_Lighting0,
    NXCommonRT_Lighting1,

    // ��Ļ�ռ���Ӱ
    NXCommonRT_ShadowTest,

    // ����G-Buffer�ṹ���£�
    // RT0:		CustomData				        R8G8B8A8_UNORM
    // RT1:		Normal					        R32G32B32A32_FLOAT
    // RT2:		Albedo					        R10G10B10A2_UNORM
    // RT3:		Metallic, Roughness, AO, flags	R8G8B8A8_UNORM
    NXCommonRT_GBuffer0,
    NXCommonRT_GBuffer1,
    NXCommonRT_GBuffer2,
    NXCommonRT_GBuffer3,

    NXCommonRT_PostProcessing,

    NXCommonRT_SIZE,
};

enum NXCommonTexEnum
{
    NXCommonTex_White,
    NXCommonTex_Normal,
    NXCommonTex_SIZE,
};

class NXResourceManagerBase
{
public:
	NXResourceManagerBase() {}
	virtual ~NXResourceManagerBase() {}

    virtual void OnReload() {}
    virtual void Release() {}

protected:

};