#pragma once
#include "NXShaderDefinitions.h"

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
class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUICodeEditor;
class NXSSSDiffuseProfiler;
class NXGUIMaterialShaderEditor
{
private:
	enum class BtnParamType { CBuffer, Texture, Sampler };

public:
	NXGUIMaterialShaderEditor() {};
	~NXGUIMaterialShaderEditor() {};

	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void ClearShaderErrorMessages();

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS);

	void SetGUIMaterial(NXGUIMaterial* pGUIMaterial);
	void SetGUIFileBrowser(NXGUIFileBrowser* pGUIFileBrowser);
	void SetGUICodeEditor(NXGUICodeEditor* pGUICodeEditor);

	void RequestSyncMaterialData();
	void RequestSyncMaterialCodes();
	void RequestGenerateBackup();

private:
	void OnBtnNewFunctionClicked(NXCustomMaterial* pMaterial);
	void OnBtnRemoveFunctionClicked(NXCustomMaterial* pMaterial, UINT index);

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnBtnAddTextureClicked(NXCustomMaterial* pMaterial);
	void OnBtnAddSamplerClicked(NXCustomMaterial* pMaterial);

	void OnBtnRemoveParamClicked(BtnParamType btnParamType, int index);
	void OnBtnMoveParamToPrevClicked(BtnParamType btnParamType, int index);
	void OnBtnMoveParamToNextClicked(BtnParamType btnParamType, int index);
	void OnBtnMoveParamToFirstClicked(BtnParamType btnParamType, int index);
	void OnBtnMoveParamToLastClicked(BtnParamType btnParamType, int index);
	void OnBtnRevertParamClicked(NXCustomMaterial* pMaterial, BtnParamType btnParamType, int index);
	bool OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnBtnSaveClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);
	void OnShowFuncIndexChanged(int showFuncIndex);

	void Render_Code(NXCustomMaterial* pMaterial);
	void Render_FeaturePanel(NXCustomMaterial* pMaterial);
	void Render_Complies(NXCustomMaterial* pMaterial);
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_ResourceOps(const std::string& strNameId, NXCustomMaterial* pMaterial, BtnParamType btnParamType, int cbIndex);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void Render_Params_TextureItem(const int strId, NXCustomMaterial* pMaterial, NXGUITextureData& texDisplay, int texIndex);
	void Render_Params_SamplerItem(const int strId, NXCustomMaterial* pMaterial, NXGUISamplerData& ssDisplay, int ssIndex);
	void Render_Settings(NXCustomMaterial* pMaterial);
	void Render_ErrorMessages();

	void SyncMaterialData(NXCustomMaterial* pMaterial);
	void SyncMaterialCode(NXCustomMaterial* pMaterial);
	void UpdateNSLFunctions();
	std::string GenerateNSLFunctionTitle(int index); // ���� nslFunc[index] �� title������ combo ��ʾ��

	bool FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator);
	std::string GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW);

	void GenerateBackupData();

private:
	bool m_bShowWindow = false;
	NXGUIMaterial* m_pGUIMaterial = nullptr;
	NXGUIFileBrowser* m_pFileBrowser = nullptr;
	NXGUICodeEditor* m_pGUICodeEditor = nullptr;

	std::vector<std::string> m_nslFuncs;	// ��¼ NSL ����
	std::vector<std::string> m_nslTitles;	// ������һ���ڴ��¼ NSL �����ı�ͷ
	std::vector<NXHLSLCodeRegion> m_HLSLFuncRegions; // ��¼ NSL ����ת���� HLSL ���� HLSL �ж�Ӧ���к�λ�á��������ڱ������ʱ�� CodeEditor ����ת��
	int m_showFuncIndex = 0; // ������ʾ�ĺ�������

	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	NXGUICBufferSetsData m_cbSettingsDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	// ShaderEditor �и���һ�� ԭʼGUI��� cb, tex, ss������
	// ���ڵ��������� СRevert ��ť��
	std::vector<NXGUICBufferData> m_cbInfosDisplayBackup;
	std::vector<NXGUITextureData> m_texInfosDisplayBackup;
	std::vector<NXGUISamplerData> m_ssInfosDisplayBackup;

	// ��ʾShader�Ĵ�����Ϣ
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	// ���������
	std::string m_strQuery;

	bool m_bIsDirty = false;
	bool m_bNeedSyncMaterialCode = false;
	bool m_bNeedBackup = false;
};
