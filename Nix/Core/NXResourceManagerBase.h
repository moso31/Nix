#pragma once
#include "NXInstance.h"
#include "Ntr.h"
#include "BaseDefs/CppSTLFully.h"

enum NXCommonRTEnum
{
    NXCommonRT_DepthZ,
    NXCommonRT_DepthZ_R32,
    NXCommonRT_Lighting0,
    NXCommonRT_Lighting1,

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

enum NXCommonTexEnum
{
    NXCommonTex_White,
    NXCommonTex_Normal,
    NXCommonTex_Noise2DGray_64x64,
    NXCommonTex_SIZE,
};

// Nix��Ϊ�������򷨵��Ǹ�Դͷ
class NXObject;

// scene
class NXScene;

// Textures
class NXTexture;
class NXTexture2D;
class NXTexture2DArray;
class NXTextureCube;

// Materials
class NXMaterial;
class NXEasyMaterial;
class NXCustomMaterial;

// Meshes
class NXSubMeshBase;
class NXRenderableObject;
class NXPrefab;
class NXPrimitive;

// Camera
class NXCamera;

// Sky & Lights
class NXPBRLight;
class NXPBRDistantLight;
class NXPBRPointLight;
class NXPBRSpotLight;
class NXCubeMap; 

class NXResourceManagerBase
{
public:
	NXResourceManagerBase() {}
	virtual ~NXResourceManagerBase() {}

    virtual void OnReload() {}
    virtual void Release() {}

protected:

};