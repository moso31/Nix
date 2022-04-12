#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"

class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void()> onRemove);

private:
	void OnTexAlbedoChange(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexNormalChange(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexMetallicChange(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexRoughnessChange(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexAOChange(NXPBRMaterialStandard* pPickingObjectMaterial);

	void OnTexAlbedoRemove(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexNormalRemove(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexMetallicRemove(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexRoughnessRemove(NXPBRMaterialStandard* pPickingObjectMaterial);
	void OnTexAORemove(NXPBRMaterialStandard* pPickingObjectMaterial);

	void UpdateFileBrowserParameters();

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;

	std::wstring m_whiteTexPath_test;
	std::wstring m_normalTexPath_test;
};
