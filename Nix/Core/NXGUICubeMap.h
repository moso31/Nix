#pragma once
#include <string>
#include "NXGUIFileBrowser.h"

class NXScene;
class NXCubeMap;
class NXGUIFileBrowser;
struct NXGUIAssetDragData;
class NXGUICubeMap
{
public:
	NXGUICubeMap(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUICubeMap() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void OnCubeMapTexChange(NXCubeMap* pCubeMap);
	void OnCubeMapTexDrop(NXCubeMap* pCubeMap, const std::wstring& filePath);

	void UpdateFileBrowserParameters();
	bool DropDataIsCubeMapImage(NXGUIAssetDragData* pDrugData);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;
};
