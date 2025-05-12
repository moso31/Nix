#include "NXGUIMaterialShaderEditor.h"
#include <regex>
#include "BaseDefs/DearImGui.h"

#include "NXSSSDiffuseProfile.h"
#include "NXGUICommon.h"
#include "NXGUICodeEditor.h"
#include "NXPBRMaterial.h"
#include "NXConverter.h"
#include "NXGUICommandManager.h"
#include "NXAllocatorManager.h"
#include "NXCodeProcessHelper.h"

NXGUIMaterialShaderEditor::NXGUIMaterialShaderEditor()
{
	m_pGUICodeEditor = new NXGUICodeEditor(g_imgui_font_codeEditor);
}

void NXGUIMaterialShaderEditor::Render()
{
	if (!m_bShowWindow) return;

	if (m_bIsDirty)
	{
		SyncMaterialData(m_pMaterial);
		m_bIsDirty = false;

		if (m_bNeedBackup)
		{
			GenerateBackupData();
			m_bNeedBackup = false;
		}
	}

	if (m_bNeedSyncMaterialCode)
	{
		SyncMaterialCode(m_pMaterial);
		m_bNeedSyncMaterialCode = false;
	}

	bool k = ImGui::Begin("Material Editor##material_shader_editor", &m_bShowWindow);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.1f));

	ImVec2 childWindowFullSize(ImGui::GetContentRegionAvail());
	ImVec2 childWindowTableSize(childWindowFullSize.x, std::min(childWindowFullSize.x * 0.6667f, childWindowFullSize.y - 100.0f));
	ImGui::BeginChild("##material_shader_editor_table_div", childWindowTableSize);
	if (ImGui::BeginTable("##material_shader_editor_table", 2, ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		Render_Code(m_pMaterial);

		ImGui::TableNextColumn();
		Render_FeaturePanel(m_pMaterial);

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
		m_shaderErrMsgs[i] = { std::string(), -1, -1, -1, -1 };
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
		// 即：满足(a,b) 或 (a,b-c) 两种格式的任意一种（a,b,c 表任意数字）。
		std::regex re("\\((\\d+),(\\d+)(-(\\d+))?\\)");

		std::smatch match;
		if (std::regex_search(strMsg, match, re) && match.size() > 1) 
		{
			std::string strRow = match.str(1);
			std::string strCol0 = match.str(2);
			std::string strCol1 = match.str(4);

			int row  = std::stoi(strRow);
			errMsg.col0 = std::stoi(strCol0);
			errMsg.col1 = strCol1.empty() ? errMsg.col0 : std::stoi(strCol1);

			//for (int j = 0; j < m_HLSLFuncRegions.size(); j++)
			//{
			//	const auto& funcRegion = m_HLSLFuncRegions[j];
			//	if (row >= funcRegion.firstRow && row <= funcRegion.lastRow)
			//	{
			//		errMsg.page = j;
			//		errMsg.row = row - funcRegion.firstRow;
			//		break;
			//	}
			//}
		}
		else
		{
			errMsg.page = 0;
			errMsg.row = 0;
			errMsg.col0 = 0;
			errMsg.col1 = 0;
		}
	}
}

void NXGUIMaterialShaderEditor::OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle eGUIStyle)
{
	NXMatDataCBuffer* cb = new NXMatDataCBuffer();
	cb->name = "newParam";
	cb->data = Vector4(0.0f);
	cb->size = NXGUICommon::GetValueNumOfGUIStyle(eGUIStyle);
	cb->gui.style = eGUIStyle;
	cb->gui.params = NXGUICommon::GetGUIParamsDefaultValue(eGUIStyle);

	m_guiData.AddCBuffer(cb);
	RequestGenerateBackup();
}

void NXGUIMaterialShaderEditor::OnBtnAddTextureClicked(NXCustomMaterial* pMaterial)
{
	NXMatDataTexture* tx = new NXMatDataTexture();
	tx->name = "newTexture";
	tx->pTexture = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
	m_guiData.AddTexture(tx);
	RequestGenerateBackup();
}

void NXGUIMaterialShaderEditor::OnBtnAddSamplerClicked(NXCustomMaterial* pMaterial)
{
	NXMatDataSampler* ss = new NXMatDataSampler();
	ss->name = "newSampler",
	ss->filter = NXSamplerFilter::Linear;
	ss->addressU = NXSamplerAddressMode::Wrap;
	ss->addressV = NXSamplerAddressMode::Wrap;
	ss->addressW = NXSamplerAddressMode::Wrap;
	m_guiData.AddSampler(ss);
	RequestGenerateBackup();
}

void NXGUIMaterialShaderEditor::OnBtnRemoveParamClicked(NXMatDataBase* pData)
{
	m_guiData.Remove(pData);
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToPrevClicked(NXMatDataBase* pData)
{
	m_guiData.MoveToPrev(pData);
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToNextClicked(NXMatDataBase* pData)
{
	m_guiData.MoveToNext(pData);
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToFirstClicked(NXMatDataBase* pData)
{
	m_guiData.MoveToBegin(pData);
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToLastClicked(NXMatDataBase* pData)
{
	m_guiData.MoveToEnd(pData);
}

void NXGUIMaterialShaderEditor::OnBtnRevertParamClicked(NXCustomMaterial* pMaterial, NXMatDataBase* pData)
{
	auto pBackupIt = std::find_if(m_guiDataBackup.GetAll().begin(), m_guiDataBackup.GetAll().end(),
		[pData](const NXMatDataBase* backupData) {
			return backupData->name == pData->name;
		});

	auto pBackup = (pBackupIt != m_guiDataBackup.GetAll().end()) ? *pBackupIt : nullptr;

	if (pBackup->name == pData->name) 
	{
		// 将备份数据的内容复制到当前数据中
		pData->CopyFrom(pBackup);

		if (pData->IsCBuffer())
		{
			pMaterial->RequestUpdateCBufferData(true);
		}
	}
}

bool NXGUIMaterialShaderEditor::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	// 构建 NSLParam 代码
	std::string nslParams = NXCodeProcessHelper::GenerateNSL(m_guiData);

	// 重新计算 nsl func 和 titles
	UpdateNSLFunctions();

	std::string strErrVS, strErrPS;	// 若编译Shader出错，将错误信息记录到此字符串中。
	bool bCompile = pMaterial->Recompile(m_guiData, m_guiCodes, m_guiDataBackup, m_guiCodesBackup, strErrVS, strErrPS);
	
	if (bCompile)
	{
		// 若编译成功，将错误信息清空
		ClearShaderErrorMessages();

		// 若编译成功，对 GUI材质类、GUI ShaderEditor、材质类 都 MakeDirty，
		// 确保下一帧一定会更新一次材质数据。
		pMaterial->RequestUpdateCBufferData(true);
		RequestSyncMaterialData();

		// 重新生成数据备份以用于Revert
		RequestGenerateBackup();

		// 推送到命令队列，确保其他需要更新的UI 同步更新
		NXGUICommand e(NXGUICmd_MSE_CompileSuccess);
		NXGUICommandManager::GetInstance()->PushCommand(e);
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

void NXGUIMaterialShaderEditor::OnShowFuncIndexChanged(int showFuncIndex)
{
	UpdateNSLFunctions();

	m_showFuncIndex = showFuncIndex;
	m_pGUICodeEditor->SwitchFile(showFuncIndex);
	m_pGUICodeEditor->GetFocus();
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

void NXGUIMaterialShaderEditor::Release()
{
	ReleaseBackupData();
	SafeDelete(m_pGUICodeEditor);
}

void NXGUIMaterialShaderEditor::OnBtnNewFunctionClicked(NXCustomMaterial* pMaterial)
{
	m_guiCodes.commonFuncs.data.push_back("void funcs()\n{\n\t\n}");
	m_guiCodes.commonFuncs.title.push_back("void funcs()");
}

void NXGUIMaterialShaderEditor::OnBtnRemoveFunctionClicked(NXCustomMaterial* pMaterial, UINT index)
{
	m_guiCodes.commonFuncs.data.erase(m_guiCodes.commonFuncs.data.begin() + index);
	m_guiCodes.commonFuncs.title.erase(m_guiCodes.commonFuncs.title.begin() + index);
}

void NXGUIMaterialShaderEditor::Render_Code(NXCustomMaterial* pMaterial)
{
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
	
	if (ImGui::BeginCombo("##material_shader_editor_combo_func", m_guiCodes.commonFuncs.title[m_showFuncIndex].c_str()))
	{
		for (int item = 0; item < m_guiCodes.commonFuncs.title.size(); item++)
		{
			ImGui::PushID(item);
			if (ImGui::Selectable(m_guiCodes.commonFuncs.title[item].c_str()))
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
		OnShowFuncIndexChanged((int)m_guiCodes.commonFuncs.title.size() - 1);
		m_pGUICodeEditor->Load(m_guiCodes.commonFuncs.data.back(), true, m_guiCodes.commonFuncs.title.back());
	}

	ImGui::SameLine();

	if (ImGui::ButtonEx("Remove Function##material_shader_editor_btn_removefunction"))
	{
		OnBtnRemoveFunctionClicked(pMaterial, m_showFuncIndex);

		if (m_pGUICodeEditor->RemoveFile(m_showFuncIndex))
		{
			m_showFuncIndex = std::min(m_showFuncIndex, (int)m_guiCodes.commonFuncs.data.size() - 1);
			m_pGUICodeEditor->SwitchFile(m_showFuncIndex);
			m_pGUICodeEditor->GetFocus();
		}
	}

	float fEachTextLineHeight = ImGui::GetTextLineHeight();
	static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
	// 规定 UI 至少留出 10 行代码的高度
	float fTextEditorHeight = std::max(10.0f, ImGui::GetContentRegionAvail().y / fEachTextLineHeight) * fEachTextLineHeight;
	m_pGUICodeEditor->Render();
}

void NXGUIMaterialShaderEditor::Render_FeaturePanel(NXCustomMaterial* pMaterial)
{
	Render_Complies(pMaterial);

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Parameters"))
		{
			Render_Params(pMaterial);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Settings"))
		{
			Render_Settings(pMaterial);
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
}

void NXGUIMaterialShaderEditor::Render_Complies(NXCustomMaterial* pMaterial)
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
}

void NXGUIMaterialShaderEditor::Render_Params(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	ImGui::PushID("##material_shader_editor_custom_search");
	ImGui::InputText("Search params", &m_strQuery);
	ImGui::PopID();

	ImGui::BeginChild("##material_shader_editor_custom_child");
	if (ImGui::BeginTable("##material_shader_editor_params_table", 2, ImGuiTableFlags_Resizable))
	{
		int paramCnt = 0;
		for (auto* data : m_guiData.GetAll())
		{
			// 先排除搜索框以外的
			if (data->name.find(NXConvert::s2lower(m_strQuery)) == std::string::npos) continue;

			if (auto* cbData = data->IsCBuffer())
			{
				std::string strId = "##material_shader_editor_custom_child_cbuffer_" + std::to_string(paramCnt);
				ImGui::TableNextColumn();

				Render_Params_ResourceOps(strId, pMaterial, cbData);

				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-1);
				if (ImGui::BeginCombo(strId.c_str(), g_strCBufferGUIStyle[(int)cbData->gui.style]))
				{
					for (int item = 0; item < g_strCBufferGUIStyleCount; item++)
					{
						if (ImGui::Selectable(g_strCBufferGUIStyle[item]) && item != (int)cbData->gui.style)
						{
							// 设置 GUI Style
							cbData->gui.style = GetGUIStyleFromString(g_strCBufferGUIStyle[item]);

							// 根据 GUI Style 设置GUI的拖动速度或最大最小值
							cbData->gui.params = GetGUIParamsDefaultValue(cbData->gui.style);
							break;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				Render_Params_CBufferItem(strId, pMaterial, cbData);
			}
			else if (auto* txData = data->IsTexture())
			{
				ImGui::TableNextColumn();
				std::string strId = "##material_shader_editor_custom_child_texture_" + std::to_string(paramCnt);
				Render_Params_ResourceOps(strId, pMaterial, txData);

				ImGui::TableNextColumn();
				Render_Params_TextureItem(paramCnt, pMaterial, txData);
			}
			else if (auto* ssData = data->IsSampler())
			{
				ImGui::TableNextColumn();
				std::string strId = "##material_shader_editor_custom_child_sampler_" + std::to_string(paramCnt);
				Render_Params_ResourceOps(strId, pMaterial, ssData);

				ImGui::TableNextColumn();
				Render_Params_SamplerItem(paramCnt, pMaterial, ssData);
			}

			paramCnt++;
		}
	}
	ImGui::EndTable();
	ImGui::EndChild();
}

void NXGUIMaterialShaderEditor::Render_Params_ResourceOps(const std::string& strId, NXCustomMaterial* pMaterial, NXMatDataBase* pData)
{
	std::string strNameId = strId + "_name";

	ImGui::PushItemWidth(-1);
	ImGui::InputText(strNameId.c_str(), &pData->name);
	ImGui::PopItemWidth();

	float btnSize = 22.0f;
	std::string strNameIdRemove = "-" + strNameId + "_remove";
	if (ImGui::Button(strNameIdRemove.c_str(), ImVec2(btnSize, btnSize))) { OnBtnRemoveParamClicked(pData); }
	ImGui::SameLine();

	std::string strNameIdMoveToFirst = "|<" + strNameId + "_move_to_first";
	if (ImGui::Button(strNameIdMoveToFirst.c_str(), ImVec2(btnSize, btnSize))) { OnBtnMoveParamToFirstClicked(pData); }
	ImGui::SameLine();

	std::string strNameIdMoveToPrev = "<" + strNameId + "_move_to_prev";
	if (ImGui::Button(strNameIdMoveToPrev.c_str(), ImVec2(btnSize, btnSize))) { OnBtnMoveParamToPrevClicked(pData); }
	ImGui::SameLine();

	std::string strNameIdMoveToNext = ">" + strNameId + "_move_to_next";
	if (ImGui::Button(strNameIdMoveToNext.c_str(), ImVec2(btnSize, btnSize))) { OnBtnMoveParamToNextClicked(pData); }
	ImGui::SameLine();

	std::string strNameIdMoveToLast = ">|" + strNameId + "_move_to_last";
	if (ImGui::Button(strNameIdMoveToLast.c_str(), ImVec2(btnSize, btnSize))) { OnBtnMoveParamToLastClicked(pData); }
	ImGui::SameLine();

	std::string strNameIdRevert = "Revert" + strNameId + "_revert";
	if (ImGui::Button(strNameIdRevert.c_str(), ImVec2(0.0f, btnSize)))
	{
		OnBtnRevertParamClicked(pMaterial, pData);
		pMaterial->RequestUpdateCBufferData(true);
	}
}

void NXGUIMaterialShaderEditor::Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXMatDataCBuffer* pCBuffer)
{
	float paramWidth = ImGui::GetContentRegionAvail().x - 32.0f;

	bool bDraged = false;
	std::string strName = strId + pCBuffer->name;
	int bParamsEditable = 0;
	switch (pCBuffer->gui.style)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		ImGui::PushItemWidth(paramWidth);
		bDraged |= ImGui::DragScalarN(strName.data(), ImGuiDataType_Float, pCBuffer->data, pCBuffer->size, pCBuffer->gui.params[0]);
		bParamsEditable = 1;
		ImGui::PopItemWidth();
		break;
	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
		ImGui::PushItemWidth(paramWidth);
		bDraged |= ImGui::SliderScalarN(strName.data(), ImGuiDataType_Float, pCBuffer->data, pCBuffer->size, &pCBuffer->gui.params[0], &pCBuffer->gui.params[1]);
		bParamsEditable = 2;
		ImGui::PopItemWidth();
		break;
	case NXGUICBufferStyle::Color3:
		ImGui::PushItemWidth(-1);
		bDraged |= ImGui::ColorEdit3(strName.data(), pCBuffer->data);
		ImGui::PopItemWidth();
		break;
	case NXGUICBufferStyle::Color4:
		ImGui::PushItemWidth(-1);
		bDraged |= ImGui::ColorEdit4(strName.data(), pCBuffer->data);
		ImGui::PopItemWidth();
		break;
	}

	// 如果有拖动行为通知材质更新
	if (bDraged)
	{
		pCBuffer->SyncLink();
		pMaterial->SetCBInfoMemoryData();
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
				ImGui::InputFloat("speed", pCBuffer->gui.params);
			}
			else if (bParamsEditable == 2)
			{
				ImGui::InputFloat2("min/max", pCBuffer->gui.params);
				if (pCBuffer->gui.params[1] < pCBuffer->gui.params[0]) pCBuffer->gui.params[1] = pCBuffer->gui.params[0];
			}
			ImGui::PopItemWidth();

			ImGui::EndPopup();
		}

		ImGui::PopID();
	}
}

void NXGUIMaterialShaderEditor::Render_Params_TextureItem(const int texParamId, NXCustomMaterial* pMaterial, NXMatDataTexture* pTexture)
{
	bool bChanged = false;

	auto& pTex = pTexture->pTexture;
	if (pTex.IsNull()) return;

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

		NXShVisDescHeap->PushFluid(pTex->GetSRV());
		auto& srvHandle = NXShVisDescHeap->Submit();
		if (ImGui::ImageButton("##", (ImTextureID)srvHandle.ptr, size, uv0, uv1, bg_col, tint_col))
		{
			// 2023.11.14 TODO: Add a popup to switch texture...
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
			{
				auto pDropData = (NXGUIAssetDragData*)(payload->Data);
				if (NXConvert::IsImageFileExtension(pDropData->srcPath.extension().string()))
				{
					bChanged = true;
					pTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(pTex->GetName().c_str(), pDropData->srcPath);
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();

		std::string strChildId = "##material_shader_editor_imgbtn_rightside" + std::to_string(texParamId);
		ImGui::BeginChild(strChildId.c_str(), ImVec2(200.0f, texSize));
		if (ImGui::Button("Reset##"))
		{
			bChanged = true;
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
		}

		if (bChanged)
		{
			pTexture->SyncLink();
		}

		ImGui::EndChild();
	}
	ImGui::PopID();
}

void NXGUIMaterialShaderEditor::Render_Params_SamplerItem(const int strId, NXCustomMaterial* pMaterial, NXMatDataSampler* pMatData)
{
	bool bChanged = false;

	ImGui::PushID(strId);
	{
		ImGui::PushItemWidth(100.0f);
		const static char* filterTypes[] = { "Point", "Linear", "Anisotropic" };
		if (ImGui::BeginCombo("Filter##material_shader_editor_sampler_combo", filterTypes[(int)pMatData->filter]))
		{
			for (int item = 0; item < IM_ARRAYSIZE(filterTypes); item++)
			{
				if (ImGui::Selectable(filterTypes[item]))
				{
					bChanged = true;
					pMatData->filter = (NXSamplerFilter)item;
					break;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		std::string strBtn = GetAddressModeText(pMatData->addressU, pMatData->addressV, pMatData->addressW) + "##material_shader_editor_sampler_btn_addrmode";
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
				if ((int)pMatData->addressU == addressMode && (int)pMatData->addressV == addressMode && (int)pMatData->addressW == addressMode) 
					addressMode++;

				bChanged = true;
				pMatData->addressU = (NXSamplerAddressMode)addressMode;
				pMatData->addressV = (NXSamplerAddressMode)addressMode;
				pMatData->addressW = (NXSamplerAddressMode)addressMode;
				addressMode = (addressMode + 1) % 3;
			}

			if (ImGui::BeginCombo("Address U##material_shader_editor_sampler_combo_u", addressModes[(int)pMatData->addressU]))
			{
				for (int item = 0; item < IM_ARRAYSIZE(addressModes); item++)
				{
					if (ImGui::Selectable(addressModes[item]))
					{
						bChanged = true;
						pMatData->addressU = (NXSamplerAddressMode)item;
						break;
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Address V##material_shader_editor_sampler_combo_v", addressModes[(int)pMatData->addressV]))
			{
				for (int item = 0; item < IM_ARRAYSIZE(addressModes); item++)
				{
					if (ImGui::Selectable(addressModes[item]))
					{
						bChanged = true;
						pMatData->addressV = (NXSamplerAddressMode)item;
						break;
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Address W##material_shader_editor_sampler_combo_w", addressModes[(int)pMatData->addressW]))
			{
				for (int item = 0; item < IM_ARRAYSIZE(addressModes); item++)
				{
					if (ImGui::Selectable(addressModes[item]))
					{
						bChanged = true;
						pMatData->addressW = (NXSamplerAddressMode)item;
						break;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::EndPopup();

			ImGui::PopItemWidth();
		}

		if (bChanged)
		{
			pMatData->SyncLink();
		}
	}
	ImGui::PopID();
}

void NXGUIMaterialShaderEditor::Render_Settings(NXCustomMaterial* pMaterial)
{
	const static char* lightingModes[] = { "StandardLit", "Unlit", "Burley SSS" };
	uint32_t& shadingModel = m_guiData.Settings().shadingModel;
	if (ImGui::BeginCombo("Lighting model##material_shader_editor_settings", lightingModes[shadingModel]))
	{
		for (UINT item = 0; item < IM_ARRAYSIZE(lightingModes); item++)
		{
			if (ImGui::Selectable(lightingModes[item]))
			{
				shadingModel = item;
				break;
			}
		}
		ImGui::EndCombo();
	}

	if (shadingModel == 2)
	{
		float itemWidth = ImGui::CalcItemWidth();
		std::string strDiffusionProfile(pMaterial->GetSSSProfilePath().empty() ? "[Default Diffuse Profile]" : pMaterial->GetSSSProfilePath().string());
		ImGui::PushItemWidth(itemWidth);
		ImGui::InputText("", &strDiffusionProfile);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Diffuse Profile"))
		{
			ImGui::OpenPopup("##material_shader_editor_settings_diffuse_profile_popup");
		}
	}

	if (ImGui::BeginPopup("##material_shader_editor_settings_diffuse_profile_popup"))
	{
		// 递归遍历 Asset 文件夹下的所有 文件，结尾是 *.nssprof 的
		auto pathes = NXGUICommon::GetFilesInFolder("D:\\NixAssets", ".nssprof");

		ImGui::BeginChild("##material_shader_editor_settings_diffuse_profile_popup_div", ImVec2(300, 300));
		if (pathes.empty())
		{
			ImGui::Text("No SSS Profile File.");
			ImGui::Text("You can create one at Content Explorer->Add.");
		}
		else
		{
			for (auto& path : pathes)
			{
				if (ImGui::Button(path.filename().string().c_str()))
				{
					m_pMaterial->SetSSSProfile(path);
					ImGui::CloseCurrentPopup();
				}
			}
		}
		ImGui::EndChild();

		ImGui::EndPopup();
	}
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
				// clear是必须的，SwitchFile 的clear不一定会执行
				m_pGUICodeEditor->ClearSelection();

				// 在此执行代码高亮标记和跳转
				OnShowFuncIndexChanged(errMsg.page);
				m_pGUICodeEditor->AddSelection(errMsg.row, true);
			}
		}
		ImGui::EndTable();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void NXGUIMaterialShaderEditor::SyncMaterialData(NXCustomMaterial* pMaterial)
{
	m_guiData.Destroy();
	m_guiData = pMaterial->GetMaterialData().Clone(true);
}

void NXGUIMaterialShaderEditor::SyncMaterialCode(NXCustomMaterial* pMaterial)
{
	// 将材质的源代码同步过来
	m_guiCodes = pMaterial->GetMaterialCode();

	// 让 CodeEditor 也刷新一次
	m_pGUICodeEditor->ClearAllFiles();
	for (int i = 0; i < m_guiCodes.commonFuncs.data.size(); i++)
	{
		m_guiCodes.commonFuncs.title[i] = GenerateNSLFunctionTitle(i);
		m_pGUICodeEditor->Load(m_guiCodes.commonFuncs.data[i], false, m_guiCodes.commonFuncs.title[i]);
	}
	m_pGUICodeEditor->RefreshAllHighLights();
}

void NXGUIMaterialShaderEditor::UpdateNSLFunctions()
{
	// 从文本编辑器提取所有代码并更新nslFunc。
	// 2023.7.29 仅需更新正在显示的代码（按现行逻辑，未显示的代码是不会被更改的）
	m_guiCodes.commonFuncs.data[m_showFuncIndex] = m_pGUICodeEditor->GetCodeText(m_showFuncIndex);
	m_guiCodes.commonFuncs.title[m_showFuncIndex] = GenerateNSLFunctionTitle(m_showFuncIndex);
}

std::string NXGUIMaterialShaderEditor::GenerateNSLFunctionTitle(int index)
{
	if (index < 0 && index >= m_guiCodes.commonFuncs.data.size()) return std::string();
	return NXConvert::GetTitleOfFunctionData(m_guiCodes.commonFuncs.data[index]);
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
	ReleaseBackupData();
	m_guiDataBackup = m_guiData.Clone();
}

void NXGUIMaterialShaderEditor::ReleaseBackupData()
{
	m_guiDataBackup.Destroy();
}
