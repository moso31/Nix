#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXShaderDefinitions.h"

class NXGUICodeEditor;
struct NXGUIAssetDragData;
class NXGUIMaterialShaderEditor;
class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr, NXGUICodeEditor* pCodeEditor = nullptr);
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();
	void Release();

	void RequestSyncMaterialData() { m_bIsDirty = true; }

	void SaveMaterialFile(NXCustomMaterial* pMaterial);

private:
	void RenderMaterialUI_Custom(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void RenderMaterialUI_Custom_Codes(NXCustomMaterial* pMaterial);

private:
	void OnBtnEditShaderClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);
	void UpdateFileBrowserParameters();

	void SyncMaterialData(NXCustomMaterial* pMaterial);

	NXGUIMaterialShaderEditor* GetShaderEditor();

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;
	NXGUICodeEditor* m_pCodeEditor;

	// 材质 Inspector 面板不需要显示 Sampler
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;

	std::string m_nslCodeDisplay;

	NXCustomMaterial* m_pLastMaterial;
	bool m_bIsDirty;
};
