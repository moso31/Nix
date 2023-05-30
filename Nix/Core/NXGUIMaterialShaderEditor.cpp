#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXPBRMaterial.h"
#include <regex>

void NXGUIMaterialShaderEditor::Render(NXCustomMaterial* pMaterial)
{
	if (!m_bShowWindow) return;

	if (m_bIsDirty)
	{
		SyncMaterialData(pMaterial);
		m_bIsDirty = false;
	}

	bool k = ImGui::Begin("Material Editor##material_shader_editor", &m_bShowWindow);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.1f));

	ImVec2 childWindowFullSize(ImGui::GetContentRegionAvail());
	ImVec2 childWindowTableSize(childWindowFullSize.x, min(childWindowFullSize.x * 0.6667f, childWindowFullSize.y - 100.0f));
	ImGui::BeginChild("##material_shader_editor_table_div", childWindowTableSize);
	if (ImGui::BeginTable("##material_shader_editor_table", 2, ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		Render_Code();

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
}

void NXGUIMaterialShaderEditor::OnBtnRevertClicked()
{
	RequestSyncMaterialData();
}

void NXGUIMaterialShaderEditor::OnBtnRemoveParamClicked(const std::string& name)
{
	std::vector<NXGUICBufferData>::iterator it;
	if (FindCBGUIData(name, it)) m_cbInfosDisplay.erase(it);
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToPrevClicked(const std::string& name)
{
	std::vector<NXGUICBufferData>::iterator it;
	if (FindCBGUIData(name, it))
	{
		if (it != m_cbInfosDisplay.begin()) 
			std::iter_swap(it, it - 1);
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToNextClicked(const std::string& name)
{
	std::vector<NXGUICBufferData>::iterator it;
	if (FindCBGUIData(name, it))
	{
		if (it != m_cbInfosDisplay.end() - 1) 
			std::iter_swap(it, it + 1);
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToFirstClicked(const std::string& name)
{
	std::vector<NXGUICBufferData>::iterator it;
	if (FindCBGUIData(name, it))
	{
		std::rotate(m_cbInfosDisplay.begin(), it, it + 1);
	}
}

void NXGUIMaterialShaderEditor::OnBtnMoveParamToLastClicked(const std::string& name)
{
	std::vector<NXGUICBufferData>::iterator it;
	if (FindCBGUIData(name, it))
	{
		std::rotate(it, it + 1, m_cbInfosDisplay.end());
	}
}

void NXGUIMaterialShaderEditor::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	// 构建 NSLParam 代码
	std::string nslParams = ConvertShaderResourceDataToNSLParam(m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay);

	std::string strErrVS, strErrPS;	// 若编译Shader出错，将错误信息记录到此字符串中。
	bool bCompile = pMaterial->Recompile(nslParams, m_nslCode, m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay, strErrVS, strErrPS);
	
	if (bCompile)
	{
		// 若编译成功，将错误信息清空
		ClearShaderErrorMessages();

		// 若编译成功，对 GUI材质类、GUI ShaderEditor、材质类 都 MakeDirty，
		// 确保下一帧一定会更新一次材质数据。
		pMaterial->RequestUpdateCBufferData();
		m_pGUIMaterial->RequestSyncMaterialData();
		RequestSyncMaterialData();
	}
	else
	{
		// 如果编译失败，将错误信息同步到 ShaderEditor
		UpdateShaderErrorMessages(strErrVS, strErrPS);
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

void NXGUIMaterialShaderEditor::SetGUIMaterial(NXGUIMaterial* pGUIMaterial)
{
	m_pGUIMaterial = pGUIMaterial;
}

void NXGUIMaterialShaderEditor::SetGUIFileBrowser(NXGUIFileBrowser* pGUIFileBrowser)
{
	m_pFileBrowser = pGUIFileBrowser;
}

void NXGUIMaterialShaderEditor::RequestSyncMaterialData()
{
	m_bIsDirty = true;
}

void NXGUIMaterialShaderEditor::Render_Code()
{
	float fEachTextLineHeight = ImGui::GetTextLineHeight();
	static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
	// 规定 UI 至少留出 10 行代码的高度
	float fTextEditorHeight = max(10.0f, ImGui::GetContentRegionAvail().y / fEachTextLineHeight) * fEachTextLineHeight;
	ImGui::InputTextMultiline("##material_shader_editor_paramview_text", &m_nslCode, ImVec2(-FLT_MIN, fTextEditorHeight), flags);
}

void NXGUIMaterialShaderEditor::Render_Params(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	int errCnt = 0;
	for (errCnt = 0; errCnt < NXGUI_ERROR_MESSAGE_MAXLIMIT; errCnt++)
		if (m_shaderErrMsgs[errCnt].data.empty()) break;

	ImVec2 btnSize(80.0f, 40.0f);

	ImVec4 btnCompileSuccessColor(0.5f, 0.8f, 0.5f, 0.7f);
	ImVec4 btnCompileErrorColor(1.0f, 0.3f, 0.3f, 0.7f);
	ImVec4 btnText(1.0f, 1.0f, 1.0f, 1.0f);
	ImVec4 btnCompileErrorText(0.7f, 0.7f, 0.7f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_Button, errCnt ? btnCompileErrorColor : btnCompileSuccessColor);
	ImGui::PushStyleColor(ImGuiCol_Text, errCnt ? btnCompileErrorText : btnText);

	if (ImGui::Button("Compile##material_shader_editor_compile", btnSize))
	{
		OnBtnCompileClicked(pMaterial);
	}

	ImGui::PopStyleColor(); // btnCompile
	ImGui::PopStyleColor(); // btnCompile

	ImGui::SameLine();
	if (ImGui::Button("Revert##material_shader_editor_parameters_add", btnSize))
		OnBtnRevertClicked();

	ImGui::SameLine();
	if (ImGui::Button("Add param##material_shader_editor_parameters_add", btnSize))
		ImGui::OpenPopup("##material_shader_editor_add_param_popup");

	if (ImGui::BeginPopup("##material_shader_editor_add_param_popup"))
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
		ImGui::EndPopup();
	}

	(ImGui::BeginChild("##material_shader_editor_custom_child"));
	{
		if (ImGui::BeginTable("##material_shader_editor_params_table", 2, ImGuiTableFlags_Resizable))
		{
			int paramCnt = 0;
			for (auto& cbDisplay : m_cbInfosDisplay)
			{
				std::string strId = "##material_shader_editor_custom_child_cbuffer_" + std::to_string(paramCnt);
				ImGui::TableNextColumn();
				std::string strNameId = strId + "_name";
				ImGui::PushItemWidth(-1);

				ImGui::InputText(strNameId.c_str(), &cbDisplay.name);

				std::string strNameIdRemove = "-" + strNameId + "_remove";
				if (ImGui::Button(strNameIdRemove.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnRemoveParamClicked(cbDisplay.name); }
				ImGui::SameLine();

				std::string strNameIdMoveToFirst = "|<" + strNameId + "_move_to_first";
				if (ImGui::Button(strNameIdMoveToFirst.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToFirstClicked(cbDisplay.name); }
				ImGui::SameLine();

				std::string strNameIdMoveToPrev = "<" + strNameId + "_move_to_prev";
				if (ImGui::Button(strNameIdMoveToPrev.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToPrevClicked(cbDisplay.name); }
				ImGui::SameLine();

				std::string strNameIdMoveToNext = ">" + strNameId + "_move_to_next";
				if (ImGui::Button(strNameIdMoveToNext.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToNextClicked(cbDisplay.name); }
				ImGui::SameLine();

				std::string strNameIdMoveToLast = ">|" + strNameId + "_move_to_last";
				if (ImGui::Button(strNameIdMoveToLast.c_str(), ImVec2(20.0f, 20.0f))) { OnBtnMoveParamToLastClicked(cbDisplay.name); }
				ImGui::SameLine();

				ImGui::PopItemWidth();

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

				Render_Params_CBufferItem(strId, pMaterial, cbDisplay);
				ImGui::PopItemWidth();

				paramCnt++;
			}

			for (auto& texDisplay : m_texInfosDisplay)
			{
				ImGui::TableNextColumn();
				ImGui::Text(texDisplay.name.c_str());

				ImGui::TableNextColumn();
				std::string strId = "##material_shader_editor_custom_child_texture_" + std::to_string(paramCnt);

				auto& pTex = texDisplay.pTexture;
				if (!pTex) continue;

				auto onTexChange = [pMaterial, &pTex, this]()
				{
					pMaterial->SetTex2D(pTex, m_pFileBrowser->GetSelected().c_str());
				};

				auto onTexRemove = [pMaterial, &pTex, this]()
				{
					pMaterial->SetTex2D(pTex, m_pFileBrowser->GetSelected().c_str());
				};

				auto onTexDrop = [pMaterial, &pTex, this](const std::wstring& dragPath)
				{
					pMaterial->SetTex2D(pTex, dragPath);
				};

				ImGui::PushID(paramCnt);
				RenderTextureIcon(pTex->GetSRV(), m_pFileBrowser, onTexChange, onTexRemove, onTexDrop);
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::Text(texDisplay.name.data());

				paramCnt++;
			}

			// 【Sampler 的部分暂时还没想好，先空着】
			for (auto& ssDisplay : m_ssInfosDisplay)
			{
				ImGui::TableNextColumn();
				ImGui::Text(ssDisplay.name.c_str());

				ImGui::TableNextColumn();
				std::string strId = "##material_shader_editor_custom_child_sampler_" + std::to_string(paramCnt++);
				ImGui::Text(ssDisplay.name.data());
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
}

void NXGUIMaterialShaderEditor::Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay)
{
	using namespace NXGUICommon;

	bool bDraged = false;
	std::string strName = strId + cbDisplay.name;

	UINT N = GetValueNumOfGUIStyle(cbDisplay.guiStyle);
	switch (cbDisplay.guiStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		bDraged |= ImGui::DragScalarN(strName.data(), ImGuiDataType_Float, cbDisplay.data, N, cbDisplay.params[0]);
		break;
	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
		bDraged |= ImGui::SliderScalarN(strName.data(), ImGuiDataType_Float, cbDisplay.data, N, &cbDisplay.params[0], &cbDisplay.params[1]);
		break;
	case NXGUICBufferStyle::Color3:
		bDraged |= ImGui::ColorEdit3(strName.data(), cbDisplay.data);
		break;
	case NXGUICBufferStyle::Color4:
		bDraged |= ImGui::ColorEdit4(strName.data(), cbDisplay.data);
		break;
	}

	if (bDraged && cbDisplay.memoryIndex != -1) // 新加的 AddParam 在点编译按钮之前不应该传给参数
	{
		// 在这里将 GUI 修改过的参数传回给材质 CBuffer，实现视觉上的变化
		UINT actualN = cbDisplay.readType; // 实际上要拷贝的字节量是 cbDisplay 初始读取的字节数量 actualN，而不是更改 GUIStyle 以后的参数数量 N
		pMaterial->SetCBInfoMemoryData(cbDisplay.memoryIndex, actualN, cbDisplay.data);
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

		// 设置 GUI Style 的拖动速度或最大最小值
		Vector2 guiParams = GetGUIParamsDefaultValue(guiStyle);

		m_cbInfosDisplay.push_back({ cbElem.name, cbElem.type, cbDataDisplay, guiStyle, guiParams, cbElem.memoryIndex });
	}

	m_texInfosDisplay.clear();
	m_texInfosDisplay.reserve(pMaterial->GetTextureCount());
	for (UINT i = 0; i < pMaterial->GetTextureCount(); i++)
		m_texInfosDisplay.push_back({ pMaterial->GetTextureName(i), pMaterial->GetTexture(i) });

	m_ssInfosDisplay.clear();
	m_ssInfosDisplay.reserve(pMaterial->GetSamplerCount());
	for (UINT i = 0; i < pMaterial->GetSamplerCount(); i++)
		m_ssInfosDisplay.push_back({ pMaterial->GetSamplerName(i), pMaterial->GetSampler(i) });

	m_nslCode = pMaterial->GetNSLCode();
}

bool NXGUIMaterialShaderEditor::FindCBGUIData(const std::string& name, std::vector<NXGUICBufferData>::iterator& oIterator)
{
	oIterator = std::find_if(m_cbInfosDisplay.begin(), m_cbInfosDisplay.end(), [&](NXGUICBufferData& cbData) { return cbData.name == name; });
	return oIterator != m_cbInfosDisplay.end();
}
