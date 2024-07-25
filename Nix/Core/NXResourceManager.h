#pragma once
#include "NXTextureResourceManager.h"
#include "NXMaterialResourceManager.h"
#include "NXMeshResourceManager.h"
#include "NXCameraResourceManager.h"
#include "NXScriptResourceManager.h"
#include "NXLightResourceManager.h"

// by Moso31 2021.12.25
// Updated by moso31 2023.3.28
// 
// 资源管理类。主要职责：
// 1. 材质、纹理等资源的统一管理。
// 2. 对资源的重新加载。
class NXResourceManager : public NXInstance<NXResourceManager>
{
public:
    NXResourceManager();
    virtual ~NXResourceManager();

    NXTextureResourceManager* GetTextureManager() { return m_pTextureManager; }
    NXMaterialResourceManager* GetMaterialManager() { return m_pMaterialManager; }
    NXMeshResourceManager* GetMeshManager() { return m_pMeshManager; }
    NXCameraResourceManager* GetCameraManager() { return m_pCameraManager; }
    NXScriptResourceManager* GetScriptManager() { return m_pScriptManager; }
    NXLightResourceManager* GetLightManager() { return m_pLightManager; }

    // 绑定Outline父子关系
    bool BindParent(NXObject* pParent, NXObject* pChild);

    void OnReload();
    void Release();

private:
    NXTextureResourceManager* m_pTextureManager;
    NXMaterialResourceManager* m_pMaterialManager;
    NXMeshResourceManager* m_pMeshManager;
    NXCameraResourceManager* m_pCameraManager;
    NXScriptResourceManager* m_pScriptManager;
    NXLightResourceManager* m_pLightManager;
};
