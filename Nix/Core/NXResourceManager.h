#pragma once
#include "NXTextureResourceManager.h"
#include "NXMaterialResourceManager.h"

// by Moso31 2021.12.25
// Updated by moso31 2023.3.28
// 
// ��Դ�����ࡣ��Ҫְ��
// 1. ���ʡ��������Դ��ͳһ����
// 2. ����Դ�����¼��ء�
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
