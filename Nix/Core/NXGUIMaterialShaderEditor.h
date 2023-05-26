#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"
#include "NXInstance.h"

// 显示Shader的错误信息，最多不允许超过 NXGUI_ERROR_MESSAGE_MAXLIMIT 条
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 20

struct NXGUIShaderErrorMessage
{
	// 具体的编译错误信息
	std::string data;

	// 出错的行号，列号左起，列号右至
	int row;
	int col0;
	int col1;
};

class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUIMaterialShaderEditor : public NXInstance<NXGUIMaterialShaderEditor>
{
public:
	// 重载 GetInstance()，因为有成员需要初始化
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

	// 更新Shader编译错误信息（编译材质出错时触发）
	void ClearShaderErrorMessages();

	// 更新Shader编译错误信息（编译材质出错时触发）
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
	std::once_flag m_onceFlag; // 用于单例初始化

	bool m_bShowWindow;
	NXGUIMaterial* m_pGUIMaterial;
	NXGUIFileBrowser* m_pFileBrowser;

	std::string m_nslCode;

	// ShaderEditor 中复制一份 原始GUI类的 cb, tex, ss参数。
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	// 显示Shader的错误信息
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];
};