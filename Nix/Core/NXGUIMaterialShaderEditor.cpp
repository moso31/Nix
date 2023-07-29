﻿#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXGUICodeEditor.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include <regex>

void NXGUIMaterialShaderEditor::Render(NXCustomMaterial* pMaterial)
{
	if (!m_bShowWindow) return;

	if (m_bIsDirty)
	{
		SyncMaterialData(pMaterial);
		m_bIsDirty = false;

		if (m_bNeedBackup)
		{
			GenerateBackupData();
			m_bNeedBackup = false;
		}
	}

	if (m_bNeedSyncMaterialCode)
	{
		SyncMaterialCode(pMaterial);
		m_bNeedSyncMaterialCode = false;
	}

	bool k = ImGui::Begin("Material Editor##material_shader_editor", &m_bShowWindow);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.1f));

	ImVec2 childWindowFullSize(ImGui::GetContentRegionAvail());
	ImVec2 childWindowTableSize(childWindowFullSize.x, min(childWindowFullSize.x * 0.6667f, childWindowFullSize.y - 100.0f));
	ImGui::BeginChild("##material_shader_editor_table_div", childWindowTableSize);
	if (ImGui::BeginTable("##material_shader_editor_table", 2, ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		Render_Code(pMaterial);

		ImGui::TableNextColumn();
		Render_Params(pMaterial);

		ImGui::EndTable(); // ##material_shader_editor_table
	}
	ImGui::EndChild(); // ##material_shader_editor_table_div

	ImVec2 childErrMsgWindowSize(childWindowFullSize.x, childWindowFullSize.y - childWindowTableSize.y);
	ImGui::BeginChild("##material_shader_editor_errmsg", childErrMsgWindowSize);
	Render_ErrorMessages();
	ImGui::EndChild(); // ##material_shader_editor_compile

	ImGui::PopStyleColor();
	ImGui::End();
}

void NXGUIMaterialShaderEditor::ClearShaderErrorMessages()
{
	for (int i = 0; i < NXGUI_ERROR_MESSAGE_MAXLIMIT; i++)
		m_shaderErrMsgs[i] = { std::string(), -1, -1, -1 };
}

void NXGUIMaterialShaderEditor::UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS)
{
	std::istringstream iss(strCompileErrorPS);
	int lineCount = 0;

	for (int i = 0; i < NXGUI_ERROR_MESSAGE_MAXLIMIT; i++)
	{
		auto& errMsg = m_shaderErrMsgs[i];
		auto& strMsg = errMsg.data;
		std::getline(iss, errMsg.data);

		// 创建一个正则表达式对象，用于匹配括号中的行列号
		std::regex re("\\((\\d+),(\\d+)-(\\d+)\\)");

		std::smatch match;
		if (std::regex_search(strMsg, match, re) && match.size() > 1) 
		{
			std::string row_number_str = match.str(1);
			std::string start_col_number_str = match.str(2);
			std::string end_col_number_str = match.str(3);

			errMsg.row  = std::stoi(row_number_str);
			errMsg.col0 = std::stoi(start_col_number_str);
			errMsg.col1 = std::stoi(end_col_number_str);
		}
		else
		{
			errMsg.row = -1;
			errMsg.col0 = -1;
			errMsg.col1 = -1;
		}
	}
}

void NXGUIMaterialShaderEditor::OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle eGUIStyle)
{
	using namespace NXGUICommon;
	m_cbInfosDisplay.push_back({ "newParam", NXCBufferInputType::Float4, Vector4(0.0f), eGUIStyle, GetGUIParamsDefaultValue(eGUIStyle), -1 });
	RequestGenerateBackup();
}

void NXGUIMaterialShaderEditor::OnBtnAddTextureClicked(NXCustomMaterial* pMaterial)
{
	auto pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
	pTex->AddRef();
	m_texInfosDisplay.push_back({ "newTexture", NXGUITextureType::Default, pTex });
	RequestGenerateBackup();
}

void NXGUIMaterialShaderEditor::OnBtnAddSamplerClicked(NXCustomMaterial* pMaterial)
{
	m_ssInfosDisplay.push_back({ "newSampler", NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap, NXSamplerAddressMode::Wrap, NXSamplerAddressMode::Wrap });
	RequestGenerateBackup();
}

void NXGUIMaterialShaderEditor::OnBtnRemoveParamClicked(BtnParamType btnParamType, int index)
{
	switch (btnParamType)
	{
		case BtnParamType::CBuffer: m_cbInfosDisplay.erase(m_cbInfosDisplay.begin() + index); break;
		case BtnParamType::Texture: m_texInfosDisplay.erase(m_texInfosDisplay.begin() + index); break;
		case BtnParamType::Sampler: m_ssInfosDisplay.erase(m_ssInfosDisplay.begin() + index); break;
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToPrevClicked(BtnParamType btnParamType, int index)
{
	switch (btnParamType)
	{
		case BtnParamType::CBuffer: if (index != 0) std::iter_swap(m_cbInfosDisplay.begin()  + index, m_cbInfosDisplay.begin()  + index - 1); break;
		case BtnParamType::Texture: if (index != 0) std::iter_swap(m_texInfosDisplay.begin() + index, m_texInfosDisplay.begin() + index - 1); break;
		case BtnParamType::Sampler: if (index != 0) std::iter_swap(m_ssInfosDisplay.begin()  + index, m_ssInfosDisplay.begin()  + index - 1); break;
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToNextClicked(BtnParamType btnParamType, int index)
{
	switch (btnParamType)
	{
		case BtnParamType::CBuffer: if (index != m_cbInfosDisplay.size()  - 1) std::iter_swap(m_cbInfosDisplay.begin()  + index, m_cbInfosDisplay.begin()  + index + 1); break;
		case BtnParamType::Texture: if (index != m_texInfosDisplay.size() - 1) std::iter_swap(m_texInfosDisplay.begin() + index, m_texInfosDisplay.begin() + index + 1); break;
		case BtnParamType::Sampler: if (index != m_ssInfosDisplay.size()  - 1) std::iter_swap(m_ssInfosDisplay.begin()  + index, m_ssInfosDisplay.begin()  + index + 1); break;
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToFirstClicked(BtnParamType btnParamType, int index)
{
	switch (btnParamType)
	{
		case BtnParamType::CBuffer: if (index != 0) std::rotate(m_cbInfosDisplay.begin()  , m_cbInfosDisplay.begin()  + index, m_cbInfosDisplay.begin()  + index + 1); break;
		case BtnParamType::Texture: if (index != 0) std::rotate(m_texInfosDisplay.begin() , m_texInfosDisplay.begin() + index, m_texInfosDisplay.begin() + index + 1); break;
		case BtnParamType::Sampler: if (index != 0) std::rotate(m_ssInfosDisplay.begin()  , m_ssInfosDisplay.begin()  + index, m_ssInfosDisplay.begin()  + index + 1); break;
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToLastClicked(BtnParamType btnParamType, int index)
{
	switch (btnParamType)
	{
		case BtnParamType::CBuffer: if (index != m_cbInfosDisplay.size()  - 1) std::rotate(m_cbInfosDisplay.begin()  + index, m_cbInfosDisplay.begin()  + index + 1, m_cbInfosDisplay.end()); break;
		case BtnParamType::Texture: if (index != m_texInfosDisplay.size() - 1) std::rotate(m_texInfosDisplay.begin() + index, m_texInfosDisplay.begin() + index + 1, m_texInfosDisplay.end()); break;
		case BtnParamType::Sampler: if (index != m_ssInfosDisplay.size()  - 1) std::rotate(m_ssInfosDisplay.begin()  + index, m_ssInfosDisplay.begin()  + index + 1, m_ssInfosDisplay.end()); break;
	}
}

void NXGUIMaterialShaderEditor::OnBtnRevertParamClicked(NXCustomMaterial* pMaterial, BtnParamType btnParamType, int index)
{
	if (btnParamType == BtnParamType::CBuffer)
	{
		int cbBackupIdx = m_cbInfosDisplay[index].backupIndex;
		if (cbBackupIdx != -1)
		{
			m_cbInfosDisplay[index] = m_cbInfosDisplayBackup[cbBackupIdx];

			// 如果是 cb参数 revert，则还会实时更新材质的渲染状态
			auto& cbDisplay = m_cbInfosDisplay[index];
			if (cbDisplay.memoryIndex != -1) // 新加的 AddParam 在点编译按钮之前不应该传给参数
			{
				// 在这里将 GUI 修改过的参数传回给材质 CBuffer，实现视觉上的变化
				UINT actualByteCount = cbDisplay.readType;
				pMaterial->SetCBInfoMemoryData(cbDisplay.memoryIndex, actualByteCount, cbDisplay.data);

				// 通知材质下一帧更新 CBuffer
				pMaterial->RequestUpdateCBufferData();
			}
		}
	}
	else if (btnParamType == BtnParamType::Texture)
	{
		int texBackupIdx = m_texInfosDisplay[index].backupIndex;
		if (texBackupIdx != -1)
			m_texInfosDisplay[index] = m_texInfosDisplayBackup[texBackupIdx];
	}
	else if (btnParamType == BtnParamType::Sampler)
	{
		int ssBackupIdx = m_ssInfosDisplay[index].backupIndex;
		if (ssBackupIdx != -1)
			m_ssInfosDisplay[index] = m_ssInfosDisplayBackup[ssBackupIdx];
	}
	else return;
}

bool NXGUIMaterialShaderEditor::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	// 构建 NSLParam 代码
	std::string nslParams = ConvertShaderResourceDataToNSLParam(m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay);

	// 重新计算 nsl func 和 titles
	UpdateNSLFunctions();

	std::string strErrVS, strErrPS;	// 若编译Shader出错，将错误信息记录到此字符串中。
	bool bCompile = pMaterial->Recompile(nslParams, m_nslFuncs, m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay, strErrVS, strErrPS);
	
	if (bCompile)
	{
		// 若编译成功，将错误信息清空
		ClearShaderErrorMessages();

		// 若编译成功，对 GUI材质类、GUI ShaderEditor、材质类 都 MakeDirty，
		// 确保下一帧一定会更新一次材质数据。
		pMaterial->RequestUpdateCBufferData();
		m_pGUIMaterial->RequestSyncMaterialData();
		RequestSyncMaterialData();

		// 重新生成数据备份以用于Revert
		RequestGenerateBackup();
	}
	else
	{
		// 如果编译失败，将错误信息同步到 ShaderEditor
		UpdateShaderErrorMessages(strErrVS, strErrPS);
	}

	return bCompile;
}

void NXGUIMaterialShaderEditor::OnBtnSaveClicked(NXCustomMaterial* pMaterial)
{
	// 编译一下
	if (OnBtnCompileClicked(pMaterial))
	{
		// 如果编译成功，再保存
		pMaterial->SaveToNSLFile();
		pMaterial->Serialize();
	}
}

void NXGUIMaterialShaderEditor::OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDataDisplay)
{
	using namespace NXGUICommon;

	// 设置 GUI Style
	cbDataDisplay.guiStyle = GetGUIStyleFromString(g_strCBufferGUIStyle[selectIndex]);

	// 根据 GUI Style 设置GUI的拖动速度或最大最小值
	cbDataDisplay.params = GetGUIParamsDefaultValue(cbDataDisplay.guiStyle);
}

void NXGUIMaterialShaderEditor::OnShowFuncIndexChanged(int showFuncIndex)
{
	UpdateNSLFunctions();

	m_showFuncIndex = showFuncIndex;
	m_pGUICodeEditor->SwitchFile(showFuncIndex);
}

void NXGUIMaterialShaderEditor::SetGUIMaterial(NXGUIMaterial* pGUIMaterial)
{
	m_pGUIMaterial = pGUIMaterial;
}

void NXGUIMaterialShaderEditor::SetGUIFileBrowser(NXGUIFileBrowser* pGUIFileBrowser)
{
	m_pFileBrowser = pGUIFileBrowser;
}

void NXGUIMaterialShaderEditor::SetGUICodeEditor(NXGUICodeEditor* pGUICodeEditor)
{
	m_pGUICodeEditor = pGUICodeEditor;
}

void NXGUIMaterialShaderEditor::RequestSyncMaterialData()
{
	m_bIsDirty = true;
}

void NXGUIMaterialShaderEditor::RequestSyncMaterialCodes()
{
	m_bNeedSyncMaterialCode = true;
}

void NXGUIMaterialShaderEditor::RequestGenerateBackup()
{
	m_bNeedBackup = true;
}

void NXGUIMaterialShaderEditor::OnBtnNewFunctionClicked(NXCustomMaterial* pMaterial)
{
	m_nslFuncs.push_back("void funcs()\n{\n\t\n}");
	m_nslTitles.push_back("void funcs()");
}

void NXGUIMaterialShaderEditor::OnBtnRemoveFunctionClicked(NXCustomMaterial* pMaterial, UINT index)
{
	m_nslFuncs.erase(m_nslFuncs.begin() + index);
	m_nslTitles.erase(m_nslTitles.begin() + index);
}

void NXGUIMaterialShaderEditor::Render_Code(NXCustomMaterial* pMaterial)
{
	if (m_nslFuncs.empty())
		return;

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
	if (ImGui::BeginCombo("##material_shader_editor_combo_func", m_nslTitles[m_showFuncIndex].c_str()))
	{
		for (int item = 0; item < m_nslTitles.size(); item++)
		{
			ImGui::PushID(item);
			if (ImGui::Selectable(m_nslTitles[item].c_str()))
			{
				OnShowFuncIndexChanged(item);
			}
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	if (ImGui::Button("New Function##material_shader_editor_btn_newfunction"))
	{
		OnBtnNewFunctionClicked(pMaterial);
		OnShowFuncIndexChanged((int)m_nslTitles.size() - 1);
		m_pGUICodeEditor->Load(m_nslFuncs.back(), true, m_nslTitles.back());
	}

	ImGui::SameLine();

	// 如果选中的是 main() 函数，禁用删除按钮
	bool bIsMainFunc = m_showFuncIndex == 0;
	if (bIsMainFunc)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}

	if (ImGui::ButtonEx("Remove Function##material_shader_editor_btn_removefunction"))
	{
		OnBtnRemoveFunctionClicked(pMaterial, m_showFuncIndex);
		OnShowFuncIndexChanged(min(m_showFuncIndex, (int)m_nslFuncs.size() - 1));
	}

	if (bIsMainFunc)
	{
		ImGui::PopStyleColor();
		ImGui::PopItemFlag();
	}

	float fEachTextLineHeight = ImGui::GetTextLineHeight();
	static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
	// 规定 UI 至少留出 10 行代码的高度
	float fTextEditorHeight = max(10.0f, ImGui::GetContentRegionAvail().y / fEachTextLineHeight) * fEachTextLineHeight;
	m_pGUICodeEditor->Render();
}

void NXGUIMaterialShaderEditor::Render_Params(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	int errCnt = 0;
	for (errCnt = 0; errCnt < NXGUI_ERROR_MESSAGE_MAXLIMIT; errCnt++)
		if (m_shaderErrMsgs[errCnt].data.empty()) break;

	ImVec2 btnSize(80.0f, 40.0f);

	ImVec4(0.5f, 0.8f, 0.5f, 0.7f);
	ImVec4 btnCompileErrorColor(1.0f, 0.3f, 0.3f, 0.7f);
	ImVec4 btnText(1.0f, 1.0f, 1.0f, 1.0f);
	ImVec4 btnCompileErrorText(0.7f, 0.7f, 0.7f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_Button, errCnt ? ImVec4(1.0f, 0.3f, 0.3f, 0.7f) : ImVec4(0.5f, 0.8f, 0.5f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_Text, errCnt ? btnCompileErrorText : btnText);

	if (ImGui::Button("Compile##material_shader_editor_compile", btnSize))
	{
		OnBtnCompileClicked(pMaterial);
	}

	ImGui::PopStyleColor(2); // btnCompile

	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, errCnt);
	ImGui::PushStyleColor(ImGuiCol_Text, errCnt ? btnCompileErrorText : btnText);
	ImGui::SameLine();
	if (ImGui::Button("Save##material_shader_editor_save", btnSize))
	{
		OnBtnSaveClicked(pMaterial);
	}
	ImGui::PopStyleColor(); // btnCompile
	ImGui::PopItemFlag();

	ImGui::SameLine();
	if (ImGui::Button("Add param##material_shader_editor_parameters_add", btnSize))
		ImGui::OpenPopup("##material_shader_editor_add_param_popup");

	if (ImGui::BeginPopup("##material_shader_editor_add_param_popup"))
	{
		if (ImGui::BeginMenu("Value##material_shader_editor_add_param_popup_value"))
		{
			// 添加参数
			for (int item = 0; item < g_strCBufferGUIStyleCount; item++)
			{
				if (ImGui::Selectable(g_strCBufferGUIStyle[item], false))
				{
					NXGUICBufferStyle guiStyle = GetGUIStyleFromString(g_strCBufferGUIStyle[item]);
					OnBtnAddParamClicked(pMaterial, guiStyle);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Texture##material_shader_editor_add_param_popup_texture"))
		{
			OnBtnAddTextureClicked(pMaterial);
		}

		if (ImGui::MenuItem("Sampler##material_shader_editor_add_param_popup_sampler"))
		{
			OnBtnAddSamplerClicked(pMaterial);
		}

		ImGui::EndPopup();
	}

	ImGui::PushID("##material_shader_editor_custom_search");
	ImGui::InputText("Search params", &m_strQuery);
	ImGui::PopID();

	ImGui::BeginChild("##material_shader_editor_custom_child");
	{
		if (ImGui::BeginTable("##material_shader_editor_params_table", 2, ImGuiTableFlags_Resizable))
		{
			int paramCnt = 0;
			for (int i = 0; i < m_cbInfosDisplay.size(); i++)
			{
				auto& cbDisplay = m_cbInfosDisplay[i];
				std::string strNameLower = NXConvert::s2lower(cbDisplay.name);
				if (strNameLower.find(NXConvert::s2lower(m_strQuery)) == std::string::npos) continue;

				std::string strId = "##material_shader_editor_custom_child_cbuffer_" + std::to_string(paramCnt);
				ImGui::TableNextColumn();

				Render_Params_ResourceOps(strId, pMaterial, BtnParamType::CBuffer, i);

				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-1);
				if (ImGui::BeginCombo(strId.c_str(), g_strCBufferGUIStyle[(int)cbDisplay.guiStyle]))
				{
					for (int item = 0; item < g_strCBufferGUIStyleCount; item++)
					{
						if (ImGui::Selectable(g_strCBufferGUIStyle[item]) && item != (int)cbDisplay.guiStyle)
						{
							OnComboGUIStyleChanged(item, cbDisplay);
							break;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				Render_Params_CBufferItem(strId, pMaterial, cbDisplay);

				paramCnt++;
			}

			for (int i = 0; i < m_texInfosDisplay.size(); i++)
			{
				auto& texDisplay = m_texInfosDisplay[i];
				std::string strNameLower = NXConvert::s2lower(texDisplay.name);
				if (strNameLower.find(NXConvert::s2lower(m_strQuery)) == std::string::npos) continue;

				ImGui::TableNextColumn();

				std::string strId = "##material_shader_editor_custom_child_texture_" + std::to_string(paramCnt);
				Render_Params_ResourceOps(strId, pMaterial, BtnParamType::Texture, i);

				ImGui::TableNextColumn();
				Render_Params_TextureItem(paramCnt, pMaterial, texDisplay, i);

				paramCnt++;
			}

			for (int i = 0; i < m_ssInfosDisplay.size(); i++)
			{
				auto& ssDisplay = m_ssInfosDisplay[i];
				std::string strNameLower = NXConvert::s2lower(ssDisplay.name);
				if (strNameLower.find(NXConvert::s2lower(m_strQuery)) == std::string::npos) continue;

				ImGui::TableNextColumn();
				std::string strId = "##material_shader_editor_custom_child_sampler_" + std::to_string(paramCnt);
				Render_Params_ResourceOps(strId, pMaterial, BtnParamType::Sampler, i);

				ImGui::TableNextColumn();
				Render_Params_SamplerItem(paramCnt, pMaterial, ssDisplay, i);

				paramCnt++;
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
}

void NXGUIMaterialShaderEditor::Render_Params_ResourceOps(const std::string& strId, NXCustomMaterial* pMaterial, BtnParamType btnParamType, int index)
{
	std::string strNameId = strId + "_name";

	ImGui::PushItemWidth(-1);
	switch (btnParamType)
	{
	case BtnParamType::CBuffer:
		ImGui::InputText(strNameId.c_str(), &m_cbInfosDisplay[index].name);
		break;
	case BtnParamType::Texture:
		ImGui::InputText(strNameId.c_str(), &m_texInfosDisplay[index].name);
		break;
	case BtnParamType::Sampler:
		ImGui::InputText(strNameId.c_str(), &m_ssInfosDisplay[index].name);
		break;
	}
	ImGui::PopItemWidth();

	std::string strNameIdRemove = "-" + strNameId + "_remove";
	if (ImGui::Button(strNameIdRemove.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnRemoveParamClicked(btnParamType, index); }
	ImGui::SameLine();

	std::string strNameIdMoveToFirst = "|<" + strNameId + "_move_to_first";
	if (ImGui::Button(strNameIdMoveToFirst.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToFirstClicked(btnParamType, index); }
	ImGui::SameLine();

	std::string strNameIdMoveToPrev = "<" + strNameId + "_move_to_prev";
	if (ImGui::Button(strNameIdMoveToPrev.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToPrevClicked(btnParamType, index); }
	ImGui::SameLine();

	std::string strNameIdMoveToNext = ">" + strNameId + "_move_to_next";
	if (ImGui::Button(strNameIdMoveToNext.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToNextClicked(btnParamType, index); }
	ImGui::SameLine();

	std::string strNameIdMoveToLast = ">|" + strNameId + "_move_to_last";
	if (ImGui::Button(strNameIdMoveToLast.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToLastClicked(btnParamType, index); }
	ImGui::SameLine();

	std::string strNameIdRevert = "Revert" + strNameId + "_revert";
	if (ImGui::Button(strNameIdRevert.c_str())) 
	{
		OnBtnRevertParamClicked(pMaterial, btnParamType, index);
		pMaterial->RequestUpdateCBufferData();
	}
	ImGui::SameLine();
}

void NXGUIMaterialShaderEditor::Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay)
{
	using namespace NXGUICommon;
	float paramWidth = ImGui::GetContentRegionAvail().x - 32.0f;

	bool bDraged = false;
	std::string strName = strId + cbDisplay.name;

	UINT N = GetValueNumOfGUIStyle(cbDisplay.guiStyle);
	int bParamsEditable = 0;
	switch (cbDisplay.guiStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		ImGui::PushItemWidth(paramWidth);
		bDraged |= ImGui::DragScalarN(strName.data(), ImGuiDataType_Float, cbDisplay.data, N, cbDisplay.params[0]);
		bParamsEditable = 1;
		ImGui::PopItemWidth();
		break;
	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
		ImGui::PushItemWidth(paramWidth);
		bDraged |= ImGui::SliderScalarN(strName.data(), ImGuiDataType_Float, cbDisplay.data, N, &cbDisplay.params[0], &cbDisplay.params[1]);
		bParamsEditable = 2;
		ImGui::PopItemWidth();
		break;
	case NXGUICBufferStyle::Color3:
		ImGui::PushItemWidth(-1);
		bDraged |= ImGui::ColorEdit3(strName.data(), cbDisplay.data);
		ImGui::PopItemWidth();
		break;
	case NXGUICBufferStyle::Color4:
		ImGui::PushItemWidth(-1);
		bDraged |= ImGui::ColorEdit4(strName.data(), cbDisplay.data);
		ImGui::PopItemWidth();
		break;
	}

	if (bDraged && cbDisplay.memoryIndex != -1) // 新加的 AddParam 在点编译按钮之前不应该传给参数
	{
		// 在这里将 GUI 修改过的参数传回给材质 CBuffer，实现视觉上的变化
		UINT actualN = cbDisplay.readType; // 实际上要拷贝的字节量是 cbDisplay 初始读取的字节数量 actualN，而不是更改 GUIStyle 以后的参数数量 N
		pMaterial->SetCBInfoMemoryData(cbDisplay.memoryIndex, actualN, cbDisplay.data);
	}

	ImGui::SameLine();
	if (bParamsEditable)
	{
		ImGui::PushID(strName.c_str());

		if (ImGui::Button("+##param_btn", ImVec2(22.0f, 22.0f)))
		{
			//popup
			ImGui::OpenPopup("##param_popup");
		}

		if (ImGui::BeginPopup("##param_popup"))
		{
			ImGui::PushItemWidth(100);
			if (bParamsEditable == 1)
			{
				ImGui::InputFloat("speed", cbDisplay.params);
			}
			else if (bParamsEditable == 2)
			{
				ImGui::InputFloat2("min/max", cbDisplay.params);
				if (cbDisplay.params[1] < cbDisplay.params[0]) cbDisplay.params[1] = cbDisplay.params[0];
			}
			ImGui::PopItemWidth();

			ImGui::EndPopup();
		}

		ImGui::PopID();
	}
}

void NXGUIMaterialShaderEditor::Render_Params_TextureItem(const int texParamId, NXCustomMaterial* pMaterial, NXGUITextureData& texDisplay, int texIndex)
{
	using namespace NXGUICommon;

	auto& pTex = texDisplay.pTexture;
	if (!pTex) return;

	ImGui::PushID(texParamId);
	float texSize = (float)48;

	ImGuiIO& io = ImGui::GetIO();
	{
		int frame_padding = 2;									// -1 == uses default padding (style.FramePadding)
		ImVec2 size = ImVec2(texSize, texSize);                 // Size of the image we want to make visible
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);         // Black background
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);       // No tint

		auto ImTexID = pTex->GetSRV();

		if (ImGui::ImageButton(ImTexID, size, uv0, uv1, frame_padding, bg_col, tint_col))
		{
			m_pFileBrowser->SetTitle("Material");
			m_pFileBrowser->SetTypeFilters({ ".png", ".jpg", ".bmp", ".dds", ".tga", ".tif", ".tiff" });
			m_pFileBrowser->SetPwd("D:\\NixAssets");

			auto onTexChange = [pMaterial, &texDisplay, &pTex, texIndex, this]()
			{
				pTex->RemoveRef();
				pTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(pTex->GetDebugName().c_str(), m_pFileBrowser->GetSelected());

				texDisplay.texType = pMaterial->GetTextureGUIType(texIndex);
				if (texDisplay.texType == NXGUITextureType::Unknown)
					texDisplay.texType = pTex->GetSerializationData().m_textureType == NXTextureType::NormalMap ? NXGUITextureType::Normal : NXGUITextureType::Default;
			};
			m_pFileBrowser->Open();
			m_pFileBrowser->SetOnDialogOK(onTexChange);
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
			{
				auto pDropData = (NXGUIAssetDragData*)(payload->Data);
				if (NXConvert::IsImageFileExtension(pDropData->srcPath.extension().string()))
				{
					pTex->RemoveRef();
					pTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(pTex->GetDebugName().c_str(), pDropData->srcPath);

					texDisplay.texType = pMaterial->GetTextureGUIType(texIndex);
					if (texDisplay.texType == NXGUITextureType::Unknown)
						texDisplay.texType = pTex->GetSerializationData().m_textureType == NXTextureType::NormalMap ? NXGUITextureType::Normal : NXGUITextureType::Default;
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();

		std::string strChildId = "##material_shader_editor_imgbtn_rightside" + std::to_string(texParamId);
		ImGui::BeginChild(strChildId.c_str(), ImVec2(200.0f, texSize));
		ImGui::PushID(ImTexID);
		if (ImGui::Button("Reset"))
		{
			pTex->RemoveRef();
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(texDisplay.texType == NXGUITextureType::Normal ? NXCommonTex_Normal : NXCommonTex_White);
		}
		ImGui::PopID();

		const static char* textureTypes[] = { "Default", "Normal" };
		if (ImGui::BeginCombo("Type##material_shader_editor_texture_combo", textureTypes[(int)texDisplay.texType]))
		{
			for (int item = 0; item < IM_ARRAYSIZE(textureTypes); item++)
			{
				if (ImGui::Selectable(textureTypes[item]))
				{
					texDisplay.texType = (NXGUITextureType)item;
					break;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::EndChild();
	}
	ImGui::PopID();
}

void NXGUIMaterialShaderEditor::Render_Params_SamplerItem(const int strId, NXCustomMaterial* pMaterial, NXGUISamplerData& ssDisplay, int ssIndex)
{
	using namespace NXGUICommon;
	ImGui::PushID(strId);
	{
		ImGui::PushItemWidth(100.0f);
		const static char* filterTypes[] = { "Point", "Linear", "Anisotropic" };
		if (ImGui::BeginCombo("Filter##material_shader_editor_sampler_combo", filterTypes[(int)ssDisplay.filter]))
		{
			for (int item = 0; item < IM_ARRAYSIZE(filterTypes); item++)
			{
				if (ImGui::Selectable(filterTypes[item]))
				{
					ssDisplay.filter = (NXSamplerFilter)item;
					break;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		std::string strBtn = GetAddressModeText(ssDisplay.addressU, ssDisplay.addressV, ssDisplay.addressW) + "##material_shader_editor_sampler_btn_addrmode";
		if (ImGui::Button(strBtn.c_str()))
		{
			ImGui::OpenPopup("##material_shader_editor_sampler_btn_addrmode_popup");
		}

		if (ImGui::BeginPopup("##material_shader_editor_sampler_btn_addrmode_popup"))
		{
			ImGui::PushItemWidth(80.0f);

			const static char* addressModes[] = { "Wrap", "Mirror", "Clamp" };
			if (ImGui::Button("Switch Address Mode##material_shader_editor_sampler_btn_switch"))
			{
				static int addressMode = 0;
				if ((int)ssDisplay.addressU == addressMode && (int)ssDisplay.addressV == addressMode && (int)ssDisplay.addressW == addressMode) 
					addressMode++;

				ssDisplay.addressU = (NXSamplerAddressMode)addressMode;
				ssDisplay.addressV = (NXSamplerAddressMode)addressMode;
				ssDisplay.addressW = (NXSamplerAddressMode)addressMode;
				addressMode = (addressMode + 1) % 3;
			}

			if (ImGui::BeginCombo("Address U##material_shader_editor_sampler_combo_u", addressModes[(int)ssDisplay.addressU]))
			{
				for (int item = 0; item < IM_ARRAYSIZE(addressModes); item++)
				{
					if (ImGui::Selectable(addressModes[item]))
					{
						ssDisplay.addressU = (NXSamplerAddressMode)item;
						break;
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Address V##material_shader_editor_sampler_combo_v", addressModes[(int)ssDisplay.addressV]))
			{
				for (int item = 0; item < IM_ARRAYSIZE(addressModes); item++)
				{
					if (ImGui::Selectable(addressModes[item]))
					{
						ssDisplay.addressV = (NXSamplerAddressMode)item;
						break;
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Address W##material_shader_editor_sampler_combo_w", addressModes[(int)ssDisplay.addressW]))
			{
				for (int item = 0; item < IM_ARRAYSIZE(addressModes); item++)
				{
					if (ImGui::Selectable(addressModes[item]))
					{
						ssDisplay.addressW = (NXSamplerAddressMode)item;
						break;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::EndPopup();

			ImGui::PopItemWidth();
		}
	}
	ImGui::PopID();
}

void NXGUIMaterialShaderEditor::Render_ErrorMessages()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	if (ImGui::BeginTable("##material_shader_editor_errmsg_table", 1, ImGuiTableFlags_Resizable))
	{
		for (int i = 0; i < NXGUI_ERROR_MESSAGE_MAXLIMIT; i++)
		{
			ImGui::TableNextColumn();

			auto& errMsg = m_shaderErrMsgs[i];
			if (errMsg.data.empty()) 
				break;

			if (ImGui::SmallButton(errMsg.data.c_str()))
			{
				// 2023.5.28，原本要在这里做代码高亮的，
				// 但是 ImGui Multitext 的代码高亮极其难做所以战术放弃了……
			}
		}
		ImGui::EndTable();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void NXGUIMaterialShaderEditor::SyncMaterialData(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	m_cbInfosDisplay.clear();
	m_cbInfosDisplay.reserve(pMaterial->GetCBufferElemCount());
	for (UINT i = 0; i < pMaterial->GetCBufferElemCount(); i++)
	{
		auto& cbElem = pMaterial->GetCBufferElem(i);
		const float* cbElemData = pMaterial->GetCBInfoMemoryData(cbElem.memoryIndex);

		Vector4 cbDataDisplay(cbElemData);
		switch (cbElem.type)
		{
		case NXCBufferInputType::Float: cbDataDisplay = { cbDataDisplay.x, 0.0f, 0.0f, 0.0f }; break;
		case NXCBufferInputType::Float2: cbDataDisplay = { cbDataDisplay.x, cbDataDisplay.y, 0.0f, 0.0f }; break;
		case NXCBufferInputType::Float3: cbDataDisplay = { cbDataDisplay.x, cbDataDisplay.y, cbDataDisplay.z, 0.0f }; break;
		default: break;
		}

		// 如果cb中存了 GUIStyle，优先使用 GUIStyle 显示 cb
		NXGUICBufferStyle guiStyle = pMaterial->GetCBGUIStyles(i);
		if (guiStyle == NXGUICBufferStyle::Unknown)
		{
			// 否则基于 cbElem 的类型自动生成 GUIStyle
			guiStyle = GetDefaultGUIStyleFromCBufferType(cbElem.type);
		}

		// 获取 GUI Style 的拖动速度或最大最小值
		Vector2 guiParams = pMaterial->GetCBGUIParams(i);

		m_cbInfosDisplay.push_back({ cbElem.name, cbElem.type, cbDataDisplay, guiStyle, guiParams, cbElem.memoryIndex });
	}

	for (auto& texDisplay : m_texInfosDisplay)
	{
		if (texDisplay.pTexture)
			texDisplay.pTexture->RemoveRef();
	}
	m_texInfosDisplay.clear();
	m_texInfosDisplay.reserve(pMaterial->GetTextureCount());
	for (UINT i = 0; i < pMaterial->GetTextureCount(); i++)
	{
		NXTexture* pTex = pMaterial->GetTexture(i);
		pTex->AddRef();

		NXGUITextureType texType = pMaterial->GetTextureGUIType(i);
		if (texType == NXGUITextureType::Unknown)
			texType = pTex->GetSerializationData().m_textureType == NXTextureType::NormalMap ? NXGUITextureType::Normal : NXGUITextureType::Default;

		m_texInfosDisplay.push_back({ pMaterial->GetTextureName(i), texType, pTex });
	}

	m_ssInfosDisplay.clear();
	m_ssInfosDisplay.reserve(pMaterial->GetSamplerCount());
	for (UINT i = 0; i < pMaterial->GetSamplerCount(); i++)
	{
		NXSamplerFilter ssFilter = pMaterial->GetSamplerFilter(i);
		NXSamplerAddressMode ssAddressModeU = pMaterial->GetSamplerAddressMode(i, 0);
		NXSamplerAddressMode ssAddressModeV = pMaterial->GetSamplerAddressMode(i, 1);
		NXSamplerAddressMode ssAddressModeW = pMaterial->GetSamplerAddressMode(i, 2);
		m_ssInfosDisplay.push_back({ pMaterial->GetSamplerName(i), ssFilter, ssAddressModeU, ssAddressModeV, ssAddressModeW });
	}
}

void NXGUIMaterialShaderEditor::SyncMaterialCode(NXCustomMaterial* pMaterial)
{
	// 将材质的源代码同步过来
	m_nslFuncs = pMaterial->GetNSLFuncs();
	m_nslTitles.resize(m_nslFuncs.size());

	// 让 CodeEditor 也刷新一次
	m_pGUICodeEditor->ClearAllFiles();
	for (int i = 0; i < m_nslFuncs.size(); i++)
	{
		m_nslTitles[i] = GenerateNSLFunctionTitle(i);
		m_pGUICodeEditor->Load(m_nslFuncs[i], false, m_nslTitles[i]);
	}
	m_pGUICodeEditor->RefreshAllHighLights();
}

void NXGUIMaterialShaderEditor::UpdateNSLFunctions()
{
	// 从文本编辑器提取所有代码并更新nslFunc。
	// 2023.7.29 仅需更新正在显示的代码（按现行逻辑，未显示的代码是不会被更改的）
	m_nslFuncs[m_showFuncIndex] = m_pGUICodeEditor->GetCodeText(m_showFuncIndex);
	m_nslTitles[m_showFuncIndex] = GenerateNSLFunctionTitle(m_showFuncIndex);
}

std::string NXGUIMaterialShaderEditor::GenerateNSLFunctionTitle(int index)
{
	if (index < 0 && index >= m_nslFuncs.size()) return std::string();
	if (index == 0) return "main()";

	bool bInCommentLine = false;

	// while 逐行读取文本
	std::string strFunc = m_nslFuncs[index];
	while (true)
	{
		std::size_t line_end = strFunc.find_first_of("\n\r", 0);
		if (line_end == std::string::npos)
			break;

		std::string line = strFunc.substr(0, line_end);
		strFunc = strFunc.substr(line_end + 1);

		std::string lineNoComment;
		if (!bInCommentLine)
		{
			// 提取无单行注释部分，即 "//" 之前的内容。
			lineNoComment = line.substr(0, line.find("//"));
			if (lineNoComment.empty()) continue;

			// 查找是否有多行注释开头 "/*"。
			// 如果有，进一步将 lineNoComment 分割成两部分
			auto nCommentMultiLineStartPos = lineNoComment.find("/*");
			if (nCommentMultiLineStartPos != std::string::npos)
			{
				lineNoComment = lineNoComment.substr(0, nCommentMultiLineStartPos);
				bInCommentLine = true;
			}
		}
		else
		{
			// 如果在多行注释中，则查找是否有多行注释结尾 "*/"。
			// 如果有，则将 bInCommentLine 设置为 false，表示多行注释已结束。
			auto commentPos = lineNoComment.find("*/");
			if (commentPos != std::string::npos)
			{
				lineNoComment = lineNoComment.substr(commentPos + 2);
				bInCommentLine = false;
			}
			else continue; // 如果在多行注释中，且没有多行注释结尾，则忽略此行。
		}

		if (lineNoComment.empty()) continue;

		// 2023.7.29 默认去掉注释后的第一行文字内容是函数名。
		return lineNoComment;
	}

	return "unknownFunction...";
}

bool NXGUIMaterialShaderEditor::FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator)
{
	oIterator = std::find_if(m_cbInfosDisplay.begin(), m_cbInfosDisplay.end(), [&](NXGUICBufferData& cbData) { return cbData.name == name; });
	return oIterator != m_cbInfosDisplay.end();
}

std::string NXGUIMaterialShaderEditor::GetAddressModeText(const NXSamplerAddressMode addrU, const NXSamplerAddressMode addrV, const NXSamplerAddressMode addrW)
{
	std::string strAddressMode = "Address mode: ";

	if (addrU == NXSamplerAddressMode::Wrap && addrV == NXSamplerAddressMode::Wrap && addrW == NXSamplerAddressMode::Wrap)
		strAddressMode += "Wrap";
	else if (addrU == NXSamplerAddressMode::Clamp && addrV == NXSamplerAddressMode::Clamp && addrW == NXSamplerAddressMode::Clamp)
		strAddressMode += "Clamp";
	else if (addrU == NXSamplerAddressMode::Mirror && addrV == NXSamplerAddressMode::Mirror && addrW == NXSamplerAddressMode::Mirror)
		strAddressMode += "Mirror";
	else if (addrU == NXSamplerAddressMode::Border && addrV == NXSamplerAddressMode::Border && addrW == NXSamplerAddressMode::Border)
		strAddressMode += "Border";
	else
		strAddressMode += "Mixed";

	return strAddressMode;
}

void NXGUIMaterialShaderEditor::GenerateBackupData()
{
	for (int i = 0; i < m_cbInfosDisplay.size(); i++)	m_cbInfosDisplay[i].backupIndex = i;
	for (int i = 0; i < m_texInfosDisplay.size(); i++)	m_texInfosDisplay[i].backupIndex = i;
	for (int i = 0; i < m_ssInfosDisplay.size(); i++)	m_ssInfosDisplay[i].backupIndex = i;

	m_cbInfosDisplayBackup.assign(m_cbInfosDisplay.begin(), m_cbInfosDisplay.end());
	m_texInfosDisplayBackup.assign(m_texInfosDisplay.begin(), m_texInfosDisplay.end());
	m_ssInfosDisplayBackup.assign(m_ssInfosDisplay.begin(), m_ssInfosDisplay.end());
}
