#pragma once
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

// 显示Shader的错误信息，最多不允许超过 NXGUI_ERROR_MESSAGE_MAXLIMIT 条
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 50

struct NXGUIShaderErrorMessage
{
	// 具体的编译错误信息
	std::string data;

	// 出错的函数页，行号，列号左起，列号右至
	int page = 0;
	int row = 0;
	int col0 = 0;
	int col1 = 0;
};

struct NXGUIShaderFunctions
{
	std::string content;
	std::string title;
};

class NXCustomMaterial;
class NXGUICodeEditor;
class NXGUIMaterialShaderEditor
{
private:
	enum class BtnParamType { CBuffer, Texture, Sampler };

public:
	NXGUIMaterialShaderEditor();
	virtual ~NXGUIMaterialShaderEditor() {};

	void SetMaterial(NXCustomMaterial* pMaterial) { m_pMaterial = pMaterial; }

	void Render();
	void Show() { m_bShowWindow = true; }

	// 更新Shader编译错误信息（编译材质出错时触发）
	void ClearShaderErrorMessages();

	// 更新Shader编译错误信息（编译材质出错时触发）
	void UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS);

	void RequestSyncMaterialData();
	void RequestSyncMaterialCodes();
	void RequestGenerateBackup();

	void Release();

private:
	void OnBtnNewFunctionClicked(NXCustomMaterial* pMaterial);
	void OnBtnRemoveFunctionClicked(NXCustomMaterial* pMaterial, UINT index);

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXMSE_CBufferStyle guiStyle);
	void OnBtnAddTextureClicked(NXCustomMaterial* pMaterial);
	void OnBtnAddSamplerClicked(NXCustomMaterial* pMaterial);

	void OnBtnRemoveParamClicked(BtnParamType btnParamType, NXMSE_BaseData* pData);
	void OnBtnMoveParamToPrevClicked(BtnParamType btnParamType, NXMSE_BaseData* pData);
	void OnBtnMoveParamToNextClicked(BtnParamType btnParamType, NXMSE_BaseData* pData);
	void OnBtnMoveParamToFirstClicked(BtnParamType btnParamType, NXMSE_BaseData* pData);
	void OnBtnMoveParamToLastClicked(BtnParamType btnParamType, NXMSE_BaseData* pData);
	void OnBtnRevertParamClicked(NXCustomMaterial* pMaterial, NXMSE_BaseData* pData);
	bool OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnBtnSaveClicked(NXCustomMaterial* pMaterial);
	void OnShowFuncIndexChanged(int showFuncIndex);

	void Render_Code(NXCustomMaterial* pMaterial);
	void Render_FeaturePanel(NXCustomMaterial* pMaterial);
	void Render_Complies(NXCustomMaterial* pMaterial);
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_ResourceOps(const std::string& strNameId, NXCustomMaterial* pMaterial, BtnParamType btnParamType, std::string& name, NXMSE_BaseData* pData);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXMSE_CBufferData* pCBuffer);
	void Render_Params_TextureItem(const int strId, NXCustomMaterial* pMaterial, NXMSE_TextureData* pTexture);
	void Render_Params_SamplerItem(const int strId, NXCustomMaterial* pMaterial, NXMSE_SamplerData* pSampler);
	void Render_Settings(NXCustomMaterial* pMaterial);
	void Render_ErrorMessages();

	void SyncMaterialData(NXCustomMaterial* pMaterial);
	void SyncMaterialCode(NXCustomMaterial* pMaterial);
	void UpdateNSLFunctions();
	std::string GenerateNSLFunctionTitle(int index); // 生成 nslFunc[index] 的 title（用于 combo 显示）

	std::string GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW);

	void GenerateBackupData();
	void ReleaseBackupData();

private:
	bool m_bShowWindow = false;
	NXCustomMaterial* m_pMaterial = nullptr;
	NXGUICodeEditor* m_pGUICodeEditor = nullptr;
	int m_showFuncIndex = 0; // 用于显示的函数索引

	// 材质参数
	NXMSEPackDatas m_guiDatas;
	NXMSEPackDatas m_guiDatasBackup;

	// 材质代码
	NXMaterialCode m_guiCodes;
	NXMaterialCode m_guiCodesBackup;

	// 显示Shader的错误信息
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	// 搜索栏相关
	std::string m_strQuery;

	bool m_bIsDirty = false;
	bool m_bNeedSyncMaterialCode = false;
	bool m_bNeedBackup = false;
};
