#pragma once
#include "NXInstance.h"
#include "Ntr.h"

enum NXCommonRTEnum
{
    NXCommonRT_DepthZ,
    NXCommonRT_Lighting0,
    NXCommonRT_Lighting1,

    NXCommonRT_SSSLighting,

    // 屏幕空间阴影
    NXCommonRT_ShadowTest,

    // 现行G-Buffer结构如下：
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