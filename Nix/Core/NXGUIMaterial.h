#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXShaderDefinitions.h"

// �� GUI �е���ʾ Style
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

	// ��ȡ CB ʱ�ĳ�ʼTypeֵ
	// �� Param �� Type �仯��ʱ�򣬿��Ա��� copy ���࣬����ָ��ƫ�ơ�
	NXCBufferInputType readType;

	// ��¼ CBֵ����ÿ�����ݶ�ʹ������ Vec4 ���档
	// ��ô����Ϊ�˱��� GUI �ı����ݸ�ʽ����������ڴ���䡣
	Vector4 data; 

	// CB �� GUI �������ʾ
	NXGUICBufferStyle guiStyle; 

	// CB �� GUI �еĸ���������������������GUI��drugSpeed, sliderMin/Max�ȵȡ�
	Vector2 params;

	// ��¼ ԭCB �� memoryIndex
	// �� GUI ֵ�����ʱ��ʹ�ô���������׷�ݲ����е�Դ����ָ�롣
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

	// ��¼ cb���� ������ʾ GUI��
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	NXCustomMaterial* m_pLastMaterial;
};
