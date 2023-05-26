#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXPBRMaterial.h"
#include <regex>

void NXGUIMaterialShaderEditor::Render(NXCustomMaterial* pMaterial)
{
	if (!m_bShowWindow) return;

	bool k = ImGui::Begin("Material Editor##material_shader_editor", &m_bShowWindow);

	ImVec2 childWindowFullSize(ImGui::GetContentRegionAvail());
	ImVec2 childWindowTableSize(childWindowFullSize.x, min(childWindowFullSize.x * 0.6667f, childWindowFullSize.y - 200.0f));
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
	if (ImGui::Button("Compile##material_shader_editor_compile"))
		OnBtnCompileClicked(pMaterial);

	Render_ErrorMessages();

	ImGui::EndChild(); // ##material_shader_editor_compile

	ImGui::End();
}

void NXGUIMaterialShaderEditor::PrepareShaderResourceData(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay)
{
	m_cbInfosDisplay.reserve(cbInfosDisplay.size());
	m_cbInfosDisplay.assign(cbInfosDisplay.begin(), cbInfosDisplay.end());

	m_texInfosDisplay.reserve(texInfosDisplay.size());
	m_texInfosDisplay.assign(texInfosDisplay.begin(), texInfosDisplay.end());

	m_ssInfosDisplay.reserve(ssInfosDisplay.size());
	m_ssInfosDisplay.assign(ssInfosDisplay.begin(), ssInfosDisplay.end());
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

void NXGUIMaterialShaderEditor::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	// 构建 NSLParam 代码
	std::string nslParams = ConvertShaderResourceDataToNSLParam(m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay);

	std::string strErrVS, strErrPS;	// 若编译Shader出错，将错误信息记录到此字符串中。
	bool bCompile = pMaterial->Recompile(nslParams, m_nslCode, m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay, strErrVS, strErrPS);

	// 无论编译是否成功，都让GUI材质类请求同步材质数据，以更新GUIMaterial
	m_pGUIMaterial->RequestSyncMaterialData();
	
	// 如果编译失败，将错误信息同步到 ShaderEditor
	bCompile ? ClearShaderErrorMessages() : UpdateShaderErrorMessages(strErrVS, strErrPS);

	// 原始材质申请更新一次CBuffer。
	pMaterial->RequestUpdateCBufferData();
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

	if (ImGui::Button("Add param##material_shader_editor_parameters_add", ImVec2(ImGui::GetContentRegionAvail().x, 20.0f)))
	{
		ImGui::OpenPopup("##material_shader_editor_add_param_popup");
	}

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
		int paramCnt = 0;
		for (auto& cbDisplay : m_cbInfosDisplay)
		{
			std::string strId = "##material_shader_editor_custom_child_cbuffer_" + std::to_string(paramCnt++);
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
		}

		for (auto& texDisplay : m_texInfosDisplay)
		{
			std::string strId = "##material_shader_editor_custom_child_texture_" + std::to_string(paramCnt++);

			auto pTex = texDisplay.pTexture;
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

			RenderTextureIcon(pTex->GetSRV(), m_pFileBrowser, onTexChange, onTexRemove, onTexDrop);

			ImGui::SameLine();
			ImGui::Text(texDisplay.name.data());
		}

		// 【Sampler 的部分暂时还没想好，先空着】
		for (auto& ssDisplay : m_ssInfosDisplay)
		{
			std::string strId = "##material_shader_editor_custom_child_sampler_" + std::to_string(paramCnt++);
			ImGui::Text(ssDisplay.name.data());
		}
	}
	ImGui::EndChild();
}

void NXGUIMaterialShaderEditor::Render_Params_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXGUICBufferData& cbDisplay)
{
	using namespace NXGUICommon;

	bool bDraged = false;
	std::string strName = cbDisplay.name + strId;

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
	auto dl = ImGui::GetWindowDrawList();
	dl->AddCircle(ImVec2(300.0f, 300.0f), 100, -1);
	dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(220.0f, 420.0f), 0xffffffff);
	dl->AddRect(ImVec2(0.0f, 0.0f), ImVec2(220.0f, 420.0f), 0xffffffff);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	if (ImGui::BeginTable("##material_shader_editor_errmsg_table", 1, ImGuiTableFlags_Resizable))
	{
		for (int i = 0; i < NXGUI_ERROR_MESSAGE_MAXLIMIT; i++)
		{
			ImGui::TableNextColumn();

			auto& errMsg = m_shaderErrMsgs[i];
			if (ImGui::SmallButton(errMsg.data.c_str()))
			{
				errMsg.data;
			}
		}
		ImGui::EndTable();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
