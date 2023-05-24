#include "NXGUICubeMap.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXResourceReloader.h"
#include "NXGUIContentExplorer.h"
#include "NXGUICommon.h"

NXGUICubeMap::NXGUICubeMap(NXScene* pScene, NXGUIFileBrowser* pFileBrowser) :
	m_pCurrentScene(pScene),
	m_pFileBrowser(pFileBrowser)
{
}

void NXGUICubeMap::Render()
{
	using namespace NXGUICommon;

	NXCubeMap* pCubeMap = m_pCurrentScene->GetCubeMap();

	ImGui::Begin("CubeMap");

	RenderTextureIcon((ImTextureID)pCubeMap->GetSRVCubeMapPreview2D(), m_pFileBrowser, std::bind(&NXGUICubeMap::OnCubeMapTexChange, this, pCubeMap), nullptr, std::bind(&NXGUICubeMap::OnCubeMapTexDrop, this, pCubeMap, std::placeholders::_1));

	ImGui::SliderFloat("Intensity", pCubeMap->GetIntensity(), 0.0f, 10.0f);

	static int x = 0;
	static const char* items[] = { "Cube Map", "Irradiance Map"};
	ImGui::Combo("Material Type", &x, items, IM_ARRAYSIZE(items));
	pCubeMap->SetIrradMode(x);


	ImGui::End();
}

void NXGUICubeMap::OnCubeMapTexChange(NXCubeMap* pCubeMap)
{
	NXResourceReloadCubeMapCommand* pCommand = new NXResourceReloadCubeMapCommand();
	pCommand->pCubeMap = pCubeMap;
	pCommand->strFilePath = m_pFileBrowser->GetSelected().c_str();
	NXResourceReloader::GetInstance()->Push(pCommand);
}

void NXGUICubeMap::OnCubeMapTexDrop(NXCubeMap* pCubeMap, const std::wstring& filePath)
{
	NXResourceReloadCubeMapCommand* pCommand = new NXResourceReloadCubeMapCommand();
	pCommand->pCubeMap = pCubeMap;
	pCommand->strFilePath = filePath.c_str();
	NXResourceReloader::GetInstance()->Push(pCommand);
}

void NXGUICubeMap::UpdateFileBrowserParameters()
{
	m_pFileBrowser->SetTitle("Material");
	m_pFileBrowser->SetTypeFilters({ ".*", ".hdr" });
	m_pFileBrowser->SetPwd("D:\\NixAssets");
}

bool NXGUICubeMap::DropDataIsCubeMapImage(NXGUIAssetDragData* pDropData)
{
	std::string strExtension = pDropData->srcPath.extension().string();
	std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), [](UCHAR c) { return std::tolower(c); });

	return strExtension == ".hdr" || strExtension == ".dds";
}
