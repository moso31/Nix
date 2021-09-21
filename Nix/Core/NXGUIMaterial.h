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

	// ������������ʹ��pMaterial�Ĳ���
	// ��ʹ��GUI������ĳ���ʵĲ���ʱ�������ô˷������³���������ʹ���˴˲��ʵ�SubMesh��
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

	// ��¼material�����Ƿ��б����dirty����
	// ����б����֡ĩ��Ҫͨ��UpdateMaterial()������SetPBRMaterial��
	bool m_bMaterialDirty;

	std::wstring m_whiteTexPath_test;
	std::wstring m_normalTexPath_test;
};
