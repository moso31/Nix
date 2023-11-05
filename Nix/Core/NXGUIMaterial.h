#pragma once
#include "NXShaderDefinitions.h"

class NXScene;
class NXMaterial;
class NXCustomMaterial;
class NXGUICodeEditor;
struct NXGUIAssetDragData;
class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene);
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

private:
	void OnBtnEditShaderClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);

	void SyncMaterialData(NXCustomMaterial* pMaterial);

private:
	NXScene* m_pCurrentScene;
	NXMaterial* m_pLastCommonPickMaterial = nullptr;

	// 材质 Inspector 面板不需要显示 Sampler
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;

	NXCustomMaterial* m_pLastMaterial = nullptr;
	bool m_bIsDirty;
};
