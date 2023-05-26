#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXPBRMaterial.h"

NXGUIMaterialShaderEditor::NXGUIMaterialShaderEditor(NXGUIMaterial* pGUIMaterial) :
	m_pGUIMaterial(pGUIMaterial),
	m_bShowWindow(false)
{
}

void NXGUIMaterialShaderEditor::Render(NXCustomMaterial* pMaterial)
{
	if (!m_bShowWindow) return;

	ImGui::Begin("Material Editor##material_shader_editor", &m_bShowWindow);

	ImVec2 childWindowFullSize(ImGui::GetContentRegionAvail());
	ImVec2 childWindowTableSize(childWindowFullSize.x, min(childWindowFullSize.x * 0.6667f, childWindowFullSize.y - 200.0f));
	if (ImGui::BeginChild("##material_shader_editor_table_div", childWindowTableSize))
	{
		if (ImGui::BeginTable("##material_shader_editor_table", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextColumn();
			Render_Code();

			ImGui::TableNextColumn();
			Render_Params(pMaterial);

			ImGui::EndTable();
		}
		ImGui::EndChild();
	}

	ImVec2 childErrMsgWindowSize(childWindowFullSize.x, childWindowFullSize.y - childWindowTableSize.y);
	if (ImGui::BeginChild("##material_shader_editor_errmsg", childErrMsgWindowSize))
	{
		if (ImGui::Button("Compile##material_shader_editor_compile"))
		{
			OnBtnCompileClicked(pMaterial);
			UpdateShaderErrorMessages();
		}

		Render_ErrorMessages();
		ImGui::EndChild();
	}

	ImGui::End();
}

void NXGUIMaterialShaderEditor::UpdateShaderErrorMessages()
{
	std::string& strErrPS = m_pGUIMaterial->m_strCompileErrorPS;
	std::istringstream iss(strErrPS);
	int lineCount = 0;

	for (int i = 0; i < NXGUI_ERROR_MESSAGE_MAXLIMIT; i++)
	{
		auto& errMsg = m_shaderErrMsgs[i];
		std::getline(iss, errMsg.data);
	}
}

void NXGUIMaterialShaderEditor::OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle eGUIStyle)
{
	using namespace NXGUICommon;
	m_pGUIMaterial->m_cbInfosDisplay.push_back({ "newParam", NXCBufferInputType::Float4, Vector4(0.0f), eGUIStyle, GetGUIParamsDefaultValue(eGUIStyle), -1 });
}

void NXGUIMaterialShaderEditor::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
	// 构建 NSLParam 代码
	std::string nslParams = m_pGUIMaterial->BuildNSLParamString();
	pMaterial->SetNSLParam(nslParams);

	// 更新 NSLCode
	pMaterial->SetNSLCode(m_pGUIMaterial->m_nslCodeDisplay);

	// 为材质记录 backup 信息
	pMaterial->GenerateInfoBackup();

	// 将 NSL 转换成 HLSL
	// 【2023.5.23 这个过程现在会重置初始化参数，需要修改】
	std::string strHLSLHead, strHLSLBody;
	pMaterial->ConvertGUIDataToHLSL(strHLSLHead, strHLSLBody, m_pGUIMaterial->m_cbInfosDisplay, m_pGUIMaterial->m_texInfosDisplay, m_pGUIMaterial->m_ssInfosDisplay);

	// 编译 HLSL
	bool bCompileSuccess = pMaterial->CompileShader(strHLSLHead, strHLSLBody, m_pGUIMaterial->m_strCompileErrorVS, m_pGUIMaterial->m_strCompileErrorPS);

	// 如果编译失败，则用备份数据恢复材质
	if (!bCompileSuccess)
		pMaterial->RecoverInfosBackup();

	// 无论编译是否成功，都将 dirty 设为 true
	m_pGUIMaterial->m_bIsDirty = true;
}

void NXGUIMaterialShaderEditor::OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData)
{
}

void NXGUIMaterialShaderEditor::Render_Code()
{
	float fEachTextLineHeight = ImGui::GetTextLineHeight();
	static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
	// 规定 UI 至少留出 10 行代码的高度，但最多
	float fTextEditorHeight = max(10.0f, ImGui::GetContentRegionAvail().y / fEachTextLineHeight) * fEachTextLineHeight;
	ImGui::InputTextMultiline("##material_shader_editor_paramview_text", &m_pGUIMaterial->m_nslCodeDisplay, ImVec2(-FLT_MIN, fTextEditorHeight), flags);
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

	if (ImGui::BeginChild("##material_shader_editor_custom_child"))
	{
		int paramCnt = 0;
		for (auto& cbDisplay : m_pGUIMaterial->m_cbInfosDisplay)
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

		for (auto& texDisplay : m_pGUIMaterial->m_texInfosDisplay)
		{
			std::string strId = "##material_shader_editor_custom_child_texture_" + std::to_string(paramCnt++);

			auto pTex = texDisplay.pTexture;
			if (!pTex) continue;

			auto& pFileBrowser = m_pGUIMaterial->m_pFileBrowser;
			auto onTexChange = [pMaterial, &pTex, pFileBrowser]()
			{
				pMaterial->SetTex2D(pTex, pFileBrowser->GetSelected().c_str());
			};

			auto onTexRemove = [pMaterial, &pTex, pFileBrowser]()
			{
				pMaterial->SetTex2D(pTex, pFileBrowser->GetSelected().c_str());
			};

			auto onTexDrop = [pMaterial, &pTex, pFileBrowser](const std::wstring& dragPath)
			{
				pMaterial->SetTex2D(pTex, dragPath);
			};

			RenderTextureIcon(pTex->GetSRV(), pFileBrowser, onTexChange, onTexRemove, onTexDrop);

			ImGui::SameLine();
			ImGui::Text(texDisplay.name.data());
		}

		// 【Sampler 的部分暂时还没想好，先空着】
		for (auto& ssDisplay : m_pGUIMaterial->m_ssInfosDisplay)
		{
			std::string strId = "##material_shader_editor_custom_child_sampler_" + std::to_string(paramCnt++);
			ImGui::Text(ssDisplay.name.data());
		}

		ImGui::EndChild();
	}
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
		// 在这里将 GUI 修改过的参数传回给材质 CBuffer，实现视觉上的变化。
		// 实际上要拷贝的字节量是 cbDisplay 初始读取的字节数量 actualN，而不是更改 GUIStyle 以后的参数数量 N
		UINT actualN = cbDisplay.readType;
		pMaterial->SetCBInfoMemoryData(cbDisplay.memoryIndex, actualN, cbDisplay.data);
		pMaterial->UpdateCBData();
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

			auto& shaderErrMsg = m_shaderErrMsgs[i];
			if (ImGui::SmallButton(shaderErrMsg.data.c_str()))
			{
			}
		}
	}
	ImGui::EndTable();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
