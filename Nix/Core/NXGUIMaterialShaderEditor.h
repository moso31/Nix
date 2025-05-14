#pragma once
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

// ��ʾShader�Ĵ�����Ϣ����಻������ NXGUI_ERROR_MESSAGE_MAXLIMIT ��
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 50

struct NXGUICodeEditorPickingData
{
	int mode = 0; // 0 : pass code; 1 : custom functions.
	int passFuncId = 0; // �ڼ���pass
	int passEntryId = 0; // ��ڵ㺯�����ͣ�0=vs, 1=ps
	int customFuncId = 0; // �ڼ���customFunc
};

struct NXGUIShaderErrorMessage
{
	// ����ı��������Ϣ
	std::string data;

	// ����ľ���ҳ��
	NXGUICodeEditorPickingData page;

	// ����ģ��кţ��к������к�����
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

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void ClearShaderErrorMessages();

	// ����Shader���������Ϣ��������ʳ���ʱ������
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

	// ���ʲ���
	NXMaterialData m_guiData;
	NXMaterialData m_guiDataBackup;

	// ���ʴ���
	NXMaterialCode m_guiCodes;
	NXMaterialCode m_guiCodesBackup;

	// ��ʾShader�Ĵ�����Ϣ
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	// ���������
	std::string m_strQuery;

	bool m_bIsDirty = false;
	bool m_bNeedSyncMaterialCode = false;
	bool m_bNeedBackup = false;
};
