#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXShaderDefinitions.h"

struct NXGUIContentExplorerButtonDrugData;

enum NXGUICustomMatParamStyle
{
	eValue,
	eSlider,
	eColor,
	eHDRColor,
	eTexture
};

struct NXGUICustomMatParamInfo
{
	std::string name;
	NXCBufferInputType type;
	NXGUICustomMatParamStyle uiStyle;
};

class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene = nullptr, NXGUIFileBrowser* pFileBrowser = nullptr);
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	void RenderMaterialUI_Standard(NXPBRMaterialStandard* pMaterial);
	void RenderMaterialUI_Translucent(NXPBRMaterialTranslucent* pMaterial);
	void RenderMaterialUI_Subsurface(NXPBRMaterialSubsurface* pMaterial);
	void RenderMaterialUI_Custom(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters();
	void RenderMaterialUI_Custom_ParamViews();
	void RenderMaterialUI_Custom_Codes();

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

	void UpdateFileBrowserParameters();

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;

	std::wstring m_whiteTexPath_test;
	std::wstring m_normalTexPath_test;

	int m_currentMaterialTypeIndex;

	std::vector<NXGUICustomMatParamInfo> m_customParamInfos;
};
