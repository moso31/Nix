#pragma once
#include "Header.h"
#include "NXShaderDefinitions.h"

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

struct NXGUIFuncTitle
{
	std::string data;
	std::string shortData;
	int strId;
};

class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUICodeEditor;
class NXGUIMaterialShaderEditor
{
private:
	enum class BtnParamType { CBuffer, Texture, Sampler };

public:
	NXGUIMaterialShaderEditor() {};
	~NXGUIMaterialShaderEditor() {};

	void Render(NXCustomMaterial* pMaterial);
	void Show() { m_bShowWindow = true; }

	// 更新Shader编译错误信息（编译材质出错时触发）
	void ClearShaderErrorMessages();

	// 更新Shader编译错误信息（编译材质出错时触发）
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
	void Render_Params(NXCustomMaterial* pMaterial);
	void Render_Params_ResourceOps(const std::string& strNameId, NXCustomMaterial* pMaterial, BtnParamType btnParamType, int cbIndex);
	void Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay);
	void Render_Params_TextureItem(const int strId, NXCustomMaterial* pMaterial, NXGUITextureData& texDisplay, int texIndex);
	void Render_Params_SamplerItem(const int strId, NXCustomMaterial* pMaterial, NXGUISamplerData& ssDisplay, int ssIndex);
	void Render_ErrorMessages();

	void SyncMaterialData(NXCustomMaterial* pMaterial);
	void SyncMaterialCode(NXCustomMaterial* pMaterial);
	void UpdateNSLFunctionsDisplay();

	bool FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator);
	std::string GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW);

	void GenerateBackupData();

private:
	bool m_bShowWindow = false;
	NXGUIMaterial* m_pGUIMaterial = nullptr;
	NXGUIFileBrowser* m_pFileBrowser = nullptr;
	NXGUICodeEditor* m_pGUICodeEditor = nullptr;

	std::vector<std::string> m_nslFuncs;
	std::vector<NXGUIFuncTitle> m_nslFuncsTitle;

	// ShaderEditor 中复制一份 原始GUI类的 cb, tex, ss参数。
	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	std::vector<NXGUICBufferData> m_cbInfosDisplayBackup;
	std::vector<NXGUITextureData> m_texInfosDisplayBackup;
	std::vector<NXGUISamplerData> m_ssInfosDisplayBackup;

	// 显示Shader的错误信息
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	// 搜索栏相关
	std::string m_strQuery = "t";

	bool m_bIsDirty = false;
	bool m_bNeedSyncMaterialCode = false;
	bool m_bNeedBackup = false;

	// 用于显示的函数索引
	// 当 m_showFuncIndex = 0 时 显示 m_nslCode 函数（Main() 函数）；
	// 当 m_showFuncIndex = i(>0)时，显示 m_nslFuncs[i - 1] 对应的函数
	int m_showFuncIndex = 0;

	// 记录当前帧是否修改了 showFuncIndex。每帧重置。
	bool m_bShowFuncChanged = false;
};