#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"

class NXGUICubeMap
{
public:
	NXGUICubeMap(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUICubeMap() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void()> onRemove);

private:
	void OnCubeMapTexChange(NXCubeMap* pCubeMap);
	void OnCubeMapTexRemove(NXCubeMap* pCubeMap);

	void UpdateFileBrowserParameters();

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;
};
