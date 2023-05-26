#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"
#include "NXInstance.h"

// ��ʾShader�Ĵ�����Ϣ����಻������100��
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 100

struct NXGUIShaderErrorMessage
{
	// ������кţ��к�
	int errLine0;
	int errLine1;

	// ����ı��������Ϣ
	std::string data;
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
			});
		return pInstance;
	}

public:
	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	void PrepareShaderResourceData(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay);
	void PrepareNSLCode(const std::string& nslCode) { m_nslCode = nslCode; }

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS);

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);

	void SetGUIMaterial(NXGUIMaterial* pGUIMaterial);
	void SetGUIFileBrowser(NXGUIFileBrowser* pGUIFileBrowser);

private:
	void Render_Code();
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void Render_ErrorMessages();

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
};