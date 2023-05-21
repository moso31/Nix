#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXShaderDefinitions.h"

// 在 GUI 中的显示 Style
enum class NXGUICBufferStyle
{
	Value,
	Value2,
	Value3,
	Value4,
	Slider,
	Slider2,
	Slider3,
	Slider4,
	Color3,
	Color4,
};

struct NXGUICBufferData
{
	const std::string& name;

	// 读取 CB 时的初始Type值
	// 当 Param 的 Type 变化的时候，可以避免 copy 过多，导致指针偏移。
	NXCBufferInputType readType;

	// 记录 CB值，但每个数据都使用最大的 Vec4 储存。
	// 这么做是为了避免 GUI 改变数据格式产生额外的内存分配。
	Vector4 data; 

	// CB 在 GUI 中如何显示
	NXGUICBufferStyle guiStyle; 

	// CB 在 GUI 中的辅助参数，比如用来控制GUI的drugSpeed, sliderMin/Max等等。
	Vector2 params;

	// 记录 原CB 的 memoryIndex
	// 当 GUI 值变更的时候，使用此索引就能追溯材质中的源数据指针。
	int memoryIndex;
};

struct NXGUIContentExplorerButtonDrugData;
class NXGUIMaterial
{
	static const char* s_strCBufferGUIStyle[];

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
	void RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters_CBufferItem(NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
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

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial);
	void OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void UpdateFileBrowserParameters();

	void SyncMaterialData(NXCustomMaterial* pMaterial);

	NXGUICBufferStyle GetGUIStyleFromString(const std::string& strTypeString);
	NXGUICBufferStyle GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType);
	UINT GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle);
	Vector2 GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle);

private:
	NXScene* m_pCurrentScene;

	NXGUIFileBrowser* m_pFileBrowser;

	std::wstring m_whiteTexPath_test;
	std::wstring m_normalTexPath_test;

	int m_currentMaterialTypeIndex;

	// 记录 cb参数 用于显示 GUI。
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	NXCustomMaterial* m_pLastMaterial;
};
