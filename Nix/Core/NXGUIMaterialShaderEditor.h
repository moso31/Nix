#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"

class NXGUIMaterial;
class NXGUIMaterialShaderEditor
{
public:
	NXGUIMaterialShaderEditor(NXGUIMaterial* pGUIMaterial);
	~NXGUIMaterialShaderEditor() {}

	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);

private:
	void Render_Code();
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);

private:
	bool m_bShowWindow;
	NXGUIMaterial* m_pGUIMaterial;
};