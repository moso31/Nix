#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"

struct NXGUIContentExplorerButtonDrugData;
class NXGUICubeMap
{
public:
	NXGUICubeMap(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUICubeMap() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void(const std::wstring&)> onDrop);

private:
	void OnCubeMapTexChange(NXCubeMap* pCubeMap);
	void OnCubeMapTexDrop(NXCubeMap* pCubeMap, const std::wstring& filePath);

	void UpdateFileBrowserParameters();
	bool DropDataIsCubeMapImage(NXGUIContentExplorerButtonDrugData* pDrugData);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;
};
