#include "NXResourceReloader.h"
#include "NXCubeMap.h"
#include "NXPBRMaterial.h"

void NXResourceReloader::Push(NXResourceReloadCommand* pCommand)
{
	m_resourceReloadCmdList.push_back(pCommand);
}

void NXResourceReloader::MarkUnusedMaterial(NXMaterial* pMaterial)
{
	m_pUnusedMaterialList.push_back(pMaterial);
}

void NXResourceReloader::Update()
{
	for (auto cmd : m_resourceReloadCmdList)
	{
		if ((NXResourceReloadCubeMapCommand*)cmd != nullptr)
		{
			auto pCubeMap = ((NXResourceReloadCubeMapCommand*)cmd)->pCubeMap;
			if (pCubeMap)
			{
				pCubeMap->Init(((NXResourceReloadCubeMapCommand*)cmd)->strFilePath);
			}
		}
		
		delete (NXResourceReloadCubeMapCommand*)cmd;
	}
	m_resourceReloadCmdList.clear();

	for (auto mat : m_pUnusedMaterialList)
	{
		SafeRelease(mat);
	}
	m_pUnusedMaterialList.clear();
}
