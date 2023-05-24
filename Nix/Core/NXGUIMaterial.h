#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXShaderDefinitions.h"

class NXGUIMaterial;
class NXGUIMaterialShaderEditor
{
public:
	NXGUIMaterialShaderEditor(NXGUIMaterial* pGUIMaterial);
	~NXGUIMaterialShaderEditor() {}

	void Render(NXCustomMaterial* pMaterial);

	void Show() { m_bShowWindow = true; }

private:
	bool m_bShowWindow;
	NXGUIMaterial* m_pGUIMaterial;
};

struct NXGUIContentExplorerButtonDrugData;
class NXGUIMaterial
{
	static const char* s_strCBufferGUIStyle[];

	friend class NXGUIMaterialShaderEditor;
public:
	NXGUIMaterial(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();
	void Release();

private:
	void RenderMaterialUI_Standard(NXPBRMaterialStandard* pMaterial);
	void RenderMaterialUI_Translucent(NXPBRMaterialTranslucent* pMaterial);
	void RenderMaterialUI_Subsurface(NXPBRMaterialSubsurface* pMaterial);
	void RenderMaterialUI_Custom(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void RenderMaterialUI_Custom_Codes(NXCustomMaterial* pMaterial);

private:
	void RenderTextureIcon(ImTextureID ImTexID, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop);

private:
	void OnTexAlbedoChange(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexNormalChange(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexMetallicChange(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexRoughnessChange(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexAOChange(NXPBRMaterialBase* pPickingObjectMaterial);

	void OnTexAlbedoRemove(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexNormalRemove(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexMetallicRemove(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexRoughnessRemove(NXPBRMaterialBase* pPickingObjectMaterial);
	void OnTexAORemove(NXPBRMaterialBase* pPickingObjectMaterial);

	void OnTexAlbedoDrop(NXPBRMaterialBase* pPickingObjectMaterial, const std::wstring& filePath);
	void OnTexNormalDrop(NXPBRMaterialBase* pPickingObjectMaterial, const std::wstring& filePath);
	void OnTexMetallicDrop(NXPBRMaterialBase* pPickingObjectMaterial, const std::wstring& filePath);
	void OnTexRoughnessDrop(NXPBRMaterialBase* pPickingObjectMaterial, const std::wstring& filePath);
	void OnTexAODrop(NXPBRMaterialBase* pPickingObjectMaterial, const std::wstring& filePath);

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle eGUIStyle);
	void OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnBtnEditShaderClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);
	void UpdateFileBrowserParameters();

	void SyncMaterialData(NXCustomMaterial* pMaterial);
	std::string BuildNSLParamString();

	NXGUICBufferStyle	GetGUIStyleFromString(const std::string& strTypeString);
	NXGUICBufferStyle	GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType);
	UINT				GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);
	Vector2				GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;

	std::wstring m_whiteTexPath_test;
	std::wstring m_normalTexPath_test;

	int m_currentMaterialTypeIndex;

	// 编译HLSL时如果Shader出错，记录到下面的字符串中。字符串最终将会用于在GUI上的错误信息显示。
	std::string m_strCompileErrorVS;
	std::string m_strCompileErrorPS;

	// 记录 cb, tex, ss参数 用于显示 GUI。
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	std::string m_nslCodeDisplay;

	NXCustomMaterial* m_pLastMaterial;
	bool m_bIsDirty;
	
	NXGUIMaterialShaderEditor* m_pGUIMaterialShaderEditor;
};
