#include "NXResourceReloader.h"
#include "NXCubeMap.h"

void NXResourceReloader::Push(NXResourceReloadCommand* pCommand)
{
	m_resourceReloadCmdList.push_back(pCommand);
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
				pCubeMap->GenerateIrradianceMap();
				pCubeMap->GeneratePreFilterMap();
			}
		}
		
		delete (NXResourceReloadCubeMapCommand*)cmd;
	}
	
	m_resourceReloadCmdList.clear();
}
