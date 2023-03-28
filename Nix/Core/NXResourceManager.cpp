#include "NXResourceManager.h"

NXResourceManager::NXResourceManager() :
	m_pTextureManager(new NXTextureResourceManager()),
	m_pMaterialManager(new NXMaterialResourceManager())
{
}

NXResourceManager::~NXResourceManager()
{
}

void NXResourceManager::OnReload()
{
	m_pTextureManager->OnReload();
	m_pMaterialManager->OnReload();
}

void NXResourceManager::Release()
{
	SafeRelease(m_pTextureManager);
	SafeRelease(m_pMaterialManager);
}
