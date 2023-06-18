#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"
#include "NXInstance.h"

// 显示Shader的错误信息，最多不允许超过 NXGUI_ERROR_MESSAGE_MAXLIMIT 条
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 50

struct NXGUIShaderErrorMessage
{
	// 具体的编译错误信息
	std::string data;

	// 出错的行号，列号左起，列号右至
	// p.s. 暂时没用上，本来是打算做标记代码的，但ImGui::MultiText坑太多了，就没实现
	int row;
	int col0;
	int col1;
};

struct NXGUIFuncItem
{
	std::string data;
	int strId;
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
			pInstance->m_bIsDirty = false;
			});
		return pInstance;
	}

public:
	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	// 更新Shader编译错误信息（编译材质出错时触发）
	void ClearShaderErrorMessages();

	// 更新Shader编译错误信息（编译材质出错时触发）
	void UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS);

	void SetGUIMaterial(NXGUIMaterial* pGUIMaterial);
	void SetGUIFileBrowser(NXGUIFileBrowser* pGUIFileBrowser);

	void RequestSyncMaterialData();

private:
	void OnBtnNewFunctionClicked(NXCustomMaterial* pMaterial);
	void OnBtnRemoveFunctionClicked(NXCustomMaterial* pMaterial, UINT index);

	void OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle guiStyle);
	void OnBtnRevertClicked();
	void OnBtnRemoveParamClicked(const std::string& name);
	void OnBtnMoveParamToPrevClicked(const std::string& name);
	void OnBtnMoveParamToNextClicked(const std::string& name);
	void OnBtnMoveParamToFirstClicked(const std::string& name);
	void OnBtnMoveParamToLastClicked(const std::string& name);
	void OnBtnCompileClicked(NXCustomMaterial* pMaterial);
	void OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData);

	void Render_Code(NXCustomMaterial* pMaterial);
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void Render_Params_TextureItem(const int strId, NXCustomMaterial* pMaterial, NXGUITextureData& texDisplay);
	void Render_ErrorMessages();

	void SyncMaterialData(NXCustomMaterial* pMaterial);
	void UpdateNSLFunctionsDisplay();

	bool FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator);

private:
	std::once_flag m_onceFlag; // 用于单例初始化

	bool m_bShowWindow;
	NXGUIMaterial* m_pGUIMaterial;
	NXGUIFileBrowser* m_pFileBrowser;

	std::string m_nslCode;
	std::vector<std::string> m_nslFuncs;
	std::vector<NXGUIFuncItem> m_nslFuncsDisplay;

	// ShaderEditor 中复制一份 原始GUI类的 cb, tex, ss参数。
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	// 显示Shader的错误信息
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	bool m_bIsDirty;
};