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
	void RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void()> onRemove);

	// 查找所有正在使用pMaterial的材质
	// 当使用GUI调整了某材质的参数时，可以用此方法更新场景中所有使用了此材质的SubMesh。
	void UpdateMaterial(NXPBRMaterial* pMaterial);

private:
	void OnTexAlbedoChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexNormalChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexMetallicChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexRoughnessChange(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexAOChange(NXPBRMaterial* pPickingObjectMaterial);

	void OnTexAlbedoRemove(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexNormalRemove(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexMetallicRemove(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexRoughnessRemove(NXPBRMaterial* pPickingObjectMaterial);
	void OnTexAORemove(NXPBRMaterial* pPickingObjectMaterial);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;

	// 记录material数据是否有变更（dirty）。
	// 如果有变更，帧末需要通过UpdateMaterial()，重新SetPBRMaterial。
	bool m_bMaterialDirty;

	std::wstring m_whiteTexPath_test;
	std::wstring m_normalTexPath_test;
};
