#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXScene.h"

class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene = nullptr, ImGui::FileBrowser* pFileBrowser = nullptr);
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void RenderTextureIcon(ImTextureID ImTexID);

private:
	NXScene* m_pCurrentScene;

	ImGui::FileBrowser* m_pFileBrowser;
};
