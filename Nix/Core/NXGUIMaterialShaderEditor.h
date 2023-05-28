#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"
#include "NXInstance.h"

// ��ʾShader�Ĵ�����Ϣ����಻������ NXGUI_ERROR_MESSAGE_MAXLIMIT ��
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 20

struct NXGUIShaderErrorMessage
{
	// ����ı��������Ϣ
	std::string data;

	// ������кţ��к������к�����
	// p.s. ��ʱû���ϣ������Ǵ�������Ǵ���ģ���ImGui::MultiText��̫���ˣ���ûʵ��
	int row;
	int col0;
	int col1;
};

class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUIMaterialShaderEditor : public NXInstance<NXGUIMaterialShaderEditor>
{
public:
	// ���� GetInstance()����Ϊ�г�Ա��Ҫ��ʼ��
	static NXGUIMaterialShaderEditor* GetInstance()
	{
		NXGUIMaterialShaderEditor* pInstance = NXInstance<NXGUIMaterialShaderEditor>::GetInstance();
		std::call_once(pInstance->m_onceFlag, [&]() { 
			pInstance->m_bShowWindow = false; 
			pInstance->m_pGUIMaterial = nullptr; 
			pInstance->m_pFileBrowser = nullptr; 
			pInstance->m_bIsDirty = false;
			});
		return pInstance;
	}

public:
	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void ClearShaderErrorMessages();

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS);

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnBtnRevertClicked();
	void OnBtnRemoveParamClicked(const std::string& name);
	void OnBtnMoveParamToPrevClicked(const std::string& name);
	void OnBtnMoveParamToNextClicked(const std::string& name);
	void OnBtnMoveParamToFirstClicked(const std::string& name);
	void OnBtnMoveParamToLastClicked(const std::string& name);
	void OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);

	void SetGUIMaterial(NXGUIMaterial* pGUIMaterial);
	void SetGUIFileBrowser(NXGUIFileBrowser* pGUIFileBrowser);

	void RequestSyncMaterialData();

private:
	void Render_Code();
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void Render_ErrorMessages();
	void SyncMaterialData(NXCustomMaterial* pMaterial);

	bool FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator);

private:
	std::once_flag m_onceFlag; // ���ڵ�����ʼ��

	bool m_bShowWindow;
	NXGUIMaterial* m_pGUIMaterial;
	NXGUIFileBrowser* m_pFileBrowser;

	std::string m_nslCode;

	// ShaderEditor �и���һ�� ԭʼGUI��� cb, tex, ss������
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	// ��ʾShader�Ĵ�����Ϣ
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	bool m_bIsDirty;
};