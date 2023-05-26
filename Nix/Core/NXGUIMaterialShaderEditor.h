#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"

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
class NXGUIMaterialShaderEditor
{
public:
	NXGUIMaterialShaderEditor(NXGUIMaterial* pGUIMaterial);
	~NXGUIMaterialShaderEditor() {}

	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	// ����Shader���������Ϣ��������ʳ���ʱ������
	void UpdateShaderErrorMessages();

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);

private:
	void Render_Code();
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void Render_ErrorMessages();

private:
	bool m_bShowWindow;
	NXGUIMaterial* m_pGUIMaterial;

	// ��ʾShader�Ĵ�����Ϣ
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];
};