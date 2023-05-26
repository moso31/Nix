#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXPBRMaterial.h"

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

void NXGUIMaterialShaderEditor::UpdateShaderErrorMessages(const std::string& strCompileErrorVS, const std::string& strCompileErrorPS)
{
	std::istringstream iss(strCompileErrorPS);
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
	m_cbInfosDisplay.push_back({ "newParam", NXCBufferInputType::Float4, Vector4(0.0f), eGUIStyle, GetGUIParamsDefaultValue(eGUIStyle), -1 });
}

void NXGUIMaterialShaderEditor::OnBtnCompileClicked(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	// ���� NSLParam ����
	std::string nslParams = ConvertShaderResourceDataToNSLParam(m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay);

	std::string strErrVS, strErrPS;	// ������Shader������������Ϣ��¼�����ַ����С�
	bool bCompile = pMaterial->Recompile(nslParams, m_nslCode, m_cbInfosDisplay, m_texInfosDisplay, m_ssInfosDisplay, strErrVS, strErrPS);

	// ���۱����Ƿ�ɹ�������GUI����������ͬ���������ݣ��Ը���GUIMaterial
	m_pGUIMaterial->RequestSyncMaterialData();

	// �������ʧ�ܣ��� Shader������Ϣͬ���� ShaderEditor
	if (!bCompile)
		UpdateShaderErrorMessages(strErrVS, strErrPS);
}

void NXGUIMaterialShaderEditor::OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDataDisplay)
{
	using namespace NXGUICommon;

	// ���� GUI Style
	cbDataDisplay.guiStyle = GetGUIStyleFromString(g_strCBufferGUIStyle[selectIndex]);

	// ���� GUI Style ����GUI���϶��ٶȻ������Сֵ
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
	// �涨 UI �������� 10 �д���ĸ߶�
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
		// ��Ӳ���
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

		// ��Sampler �Ĳ�����ʱ��û��ã��ȿ��š�
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

	if (bDraged && cbDisplay.memoryIndex != -1) // �¼ӵ� AddParam �ڵ���밴ť֮ǰ��Ӧ�ô�������
	{
		// �����ｫ GUI �޸Ĺ��Ĳ������ظ����� CBuffer��ʵ���Ӿ��ϵı仯
		UINT actualN = cbDisplay.readType; // ʵ����Ҫ�������ֽ����� cbDisplay ��ʼ��ȡ���ֽ����� actualN�������Ǹ��� GUIStyle �Ժ�Ĳ������� N
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
		ImGui::EndTable();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
