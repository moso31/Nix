#pragma once
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

// 显示Shader的错误信息，最多不允许超过 NXGUI_ERROR_MESSAGE_MAXLIMIT 条
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 50

struct NXGUICodeEditorPickingData
{
	int mode = 0; // 0 : pass code; 1 : custom functions.
	int passFuncId = 0; // 第几个pass
	int passEntryId = 0; // 入口点函数类型，0=vs, 1=ps
	int customFuncId = 0; // 第几个customFunc
};

struct NXGUIShaderErrorMessage
{
	// 具体的编译错误信息
	std::string data;

	// 出错的具体页面
	NXGUICodeEditorPickingData page;

	// 出错的，行号，列号左起，列号右至
	int row = 0;
	int col0 = 0;
	int col1 = 0;
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

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnBtnAddTextureClicked(NXCustomMaterial* pMaterial);
	void OnBtnAddSamplerClicked(NXCustomMaterial* pMaterial);

	void OnBtnRemoveParamClicked(NXMatDataBase* pData);
	void OnBtnMoveParamToPrevClicked(NXMatDataBase* pData);
	void OnBtnMoveParamToNextClicked(NXMatDataBase* pData);
	void OnBtnMoveParamToFirstClicked(NXMatDataBase* pData);
	void OnBtnMoveParamToLastClicked(NXMatDataBase* pData);
	void OnBtnRevertParamClicked(NXCustomMaterial* pMaterial, NXMatDataBase* pData);
	bool OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnBtnSaveClicked(NXCustomMaterial* pMaterial);

	void Render_Code(NXCustomMaterial* pMaterial);
	void Render_FeaturePanel(NXCustomMaterial* pMaterial);
	void Render_Complies(NXCustomMaterial* pMaterial);
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_ResourceOps(const std::string& strNameId, NXCustomMaterial* pMaterial, NXMatDataBase* pData);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXMatDataCBuffer* pCBuffer);
	void Render_Params_TextureItem(const int strId, NXCustomMaterial* pMaterial, NXMatDataTexture* pTexture);
	void Render_Params_SamplerItem(const int strId, NXCustomMaterial* pMaterial, NXMatDataSampler* pSampler);
	void Render_Settings(NXCustomMaterial* pMaterial);
	void Render_ErrorMessages();

	void SyncMaterialData(NXCustomMaterial* pMaterial);
	void SyncMaterialCode(NXCustomMaterial* pMaterial);
	void UpdateNSLFunctions();

	int GetCodeEditorIndexOfPickingData(const NXGUICodeEditorPickingData& pickingData);
	int GetEntryNum();
	void SyncLastPickingData();
	void LoadPickingCodeEditor();

	std::string GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW);

	void GenerateBackupData();
	void ReleaseBackupData();

private:
	bool m_bShowWindow = false;
	NXCustomMaterial* m_pMaterial = nullptr;
	NXGUICodeEditor* m_pGUICodeEditor = nullptr;

	NXGUICodeEditorPickingData m_pickingData;
	NXGUICodeEditorPickingData m_pickingDataLast;

	// 材质参数
	NXMaterialData m_guiData;
	NXMaterialData m_guiDataBackup;

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
