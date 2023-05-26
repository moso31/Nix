#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"

// 显示Shader的错误信息，最多不允许超过100条
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 100

struct NXGUIShaderErrorMessage
{
	// 出错的行号，列号
	int errLine0;
	int errLine1;

	// 具体的编译错误信息
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

	// 更新Shader编译错误信息（编译材质出错时触发）
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

	// 显示Shader的错误信息
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];
};