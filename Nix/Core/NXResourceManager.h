#pragma once
#include "NXTextureResourceManager.h"
#include "NXMaterialResourceManager.h"

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
    ~NXResourceManager();

    NXTextureResourceManager* GetTextureManager() { return m_pTextureManager; }
    NXMaterialResourceManager* GetMaterialManager() { return m_pMaterialManager; }

    void OnReload();
    void Release();

private:
    NXTextureResourceManager* m_pTextureManager;
    NXMaterialResourceManager* m_pMaterialManager;
};
