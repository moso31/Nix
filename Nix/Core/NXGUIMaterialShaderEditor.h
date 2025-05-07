#pragma once
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

// ��ʾShader�Ĵ�����Ϣ����಻������ NXGUI_ERROR_MESSAGE_MAXLIMIT ��
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 50

struct NXGUIShaderErrorMessage
{
	// ����ı��������Ϣ
	std::string data;

	// ����ĺ���ҳ���кţ��к������к�����
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
	std::string GenerateNSLFunctionTitle(int index); // ���� nslFunc[index] �� title������ combo ��ʾ��

	std::string GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW);

	void GenerateBackupData();
	void ReleaseBackupData();

private:
	bool m_bShowWindow = false;
	NXCustomMaterial* m_pMaterial = nullptr;
	NXGUICodeEditor* m_pGUICodeEditor = nullptr;
	int m_showFuncIndex = 0; // ������ʾ�ĺ�������

	// ���ʲ���
	NXMSEPackDatas m_guiDatas;
	NXMSEPackDatas m_guiDatasBackup;

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
