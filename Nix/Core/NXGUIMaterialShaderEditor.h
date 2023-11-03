#pragma once
#include "NXShaderDefinitions.h"

// 显示Shader的错误信息，最多不允许超过 NXGUI_ERROR_MESSAGE_MAXLIMIT 条
#define NXGUI_ERROR_MESSAGE_MAXLIMIT 50

struct NXGUIShaderErrorMessage
{
	// 具体的编译错误信息
	std::string data;

	// 出错的函数页，行号，列号左起，列号右至
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
	std::string GenerateNSLFunctionTitle(int index); // 生成 nslFunc[index] 的 title（用于 combo 显示）

	bool FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator);
	std::string GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW);

	void GenerateBackupData();

private:
	bool m_bShowWindow = false;
	NXGUIMaterial* m_pGUIMaterial = nullptr;
	NXGUIFileBrowser* m_pFileBrowser = nullptr;
	NXGUICodeEditor* m_pGUICodeEditor = nullptr;

	std::vector<std::string> m_nslFuncs;	// 记录 NSL 函数
	std::vector<std::string> m_nslTitles;	// 额外用一份内存记录 NSL 函数的标头
	std::vector<NXHLSLCodeRegion> m_HLSLFuncRegions; // 记录 NSL 函数转换到 HLSL 后，在 HLSL 中对应的行号位置。（方便在编译错误时给 CodeEditor 做跳转）
	int m_showFuncIndex = 0; // 用于显示的函数索引

	std::vector<NXGUICBufferData> m_cbInfosDisplay;
	NXGUICBufferSetsData m_cbSettingsDisplay;
	std::vector<NXGUITextureData> m_texInfosDisplay;
	std::vector<NXGUISamplerData> m_ssInfosDisplay;

	// ShaderEditor 中复制一份 原始GUI类的 cb, tex, ss参数。
	// 用于单个参数的 小Revert 按钮。
	std::vector<NXGUICBufferData> m_cbInfosDisplayBackup;
	std::vector<NXGUITextureData> m_texInfosDisplayBackup;
	std::vector<NXGUISamplerData> m_ssInfosDisplayBackup;

	// 显示Shader的错误信息
	NXGUIShaderErrorMessage m_shaderErrMsgs[NXGUI_ERROR_MESSAGE_MAXLIMIT];

	// 搜索栏相关
	std::string m_strQuery;

	bool m_bIsDirty = false;
	bool m_bNeedSyncMaterialCode = false;
	bool m_bNeedBackup = false;
};
