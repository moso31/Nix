#include "NXResourceManager.h"
#include "NXObject.h"

NXResourceManager::NXResourceManager() :
	m_pTextureManager(new NXTextureResourceManager()),
	m_pMaterialManager(new NXMaterialResourceManager()),
	m_pMeshManager(new NXMeshResourceManager()),
	m_pCameraManager(new NXCameraResourceManager()),
	m_pScriptManager(new NXScriptResourceManager()),
	m_pLightManager(new NXLightResourceManager())
{
}

NXResourceManager::~NXResourceManager()
{
}

bool NXResourceManager::BindParent(NXObject* pParent, NXObject* pChild)
{
	if (!pChild || !pParent)
		return false;

	pChild->SetParent(pParent);
	return true;
}

void NXResourceManager::OnReload()
{
	m_pTextureManager->OnReload();
	m_pMaterialManager->OnReload();
	m_pMeshManager->OnReload();
	m_pCameraManager->OnReload();
	m_pScriptManager->OnReload();
	m_pLightManager->OnReload();
}

void NXResourceManager::Release()
{
	SafeRelease(m_pTextureManager);
	SafeRelease(m_pMaterialManager);
	SafeRelease(m_pMeshManager);
	SafeRelease(m_pCameraManager);
	SafeRelease(m_pScriptManager);
	SafeRelease(m_pLightManager);
}
