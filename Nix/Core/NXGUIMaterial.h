#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXScene.h"

class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange);

private:
	void OnTexAlbedoChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexNormalChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexMetallicChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexRoughnessChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexAOChange(NXPBRMaterial* pPickingObjectMaterial);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;
};
