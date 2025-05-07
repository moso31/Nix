#pragma once
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

class NXScene;
class NXMaterial;
class NXCustomMaterial;
class NXGUICodeEditor;
struct NXGUIAssetDragData;
class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene);
	virtual ~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();
	void Release();

	void RequestSyncMaterialData() { m_bIsDirty = true; }

private:
	void RenderMaterialUI_Custom(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXMSE_CBufferData* cbData);

private:
	void OnBtnEditShaderClicked(NXCustomMaterial* pMaterial);

	void SyncMaterialData(NXCustomMaterial* pMaterial);

private:
	NXScene* m_pCurrentScene;
	NXMaterial* m_pLastCommonPickMaterial = nullptr;

	NXMSEPackDatas m_guiDataRef;

	NXCustomMaterial* m_pLastMaterial = nullptr;
	bool m_bIsDirty;
};
