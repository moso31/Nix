#pragma once
#include "NXInstance.h"
#include "Ntr.h"
#include "BaseDefs/CppSTLFully.h"
#include "NXCommonTexDefinition.h"

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
    virtual void Update() {}
    virtual void Release() {}

protected:

};