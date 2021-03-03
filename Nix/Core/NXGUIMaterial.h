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
	void UpdateMaterial(NXPBRMaterial* pMaterial);

private:
	void OnTexAlbedoChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexNormalChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexMetallicChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexRoughnessChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexAOChange(NXPBRMaterial* pPickingObjectMaterial);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;

	// 记录material数据是否有变更（dirty）。
	// 如果有变更，帧末需要通过UpdateMaterial()，重新SetPBRMaterial。
	bool m_bMaterialDirty;
};
