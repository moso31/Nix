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

	if (ImGui::Begin("Material Editor##material_shader_editor", &m_bShowWindow))
	{
		if (ImGui::BeginTable("##material_shader_editor_table", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextColumn();
			Render_Code();

			ImGui::TableNextColumn();
			Render_Params(pMaterial);

			ImGui::EndTable();
		}
		ImGui::End();
	}
}

void NXGUIMaterialShaderEditor::OnBtnAddParamClicked(NXCustomMaterial* pMaterial, NXGUICBufferStyle eGUIStyle)
{
	using namespace NXGUICommon;

	m_pGUIMaterial->m_cbInfosDisplay.push_back({ "newParam", NXCBufferInputType::Float4, Vector4(0.0f), eGUIStyle, GetGUIParamsDefaultValue(eGUIStyle), -1 });
}

void NXGUIMaterialShaderEditor::OnComboGUIStyleChanged(int selectIndex, NXGUICBufferData& cbDisplayData)
{
}

void NXGUIMaterialShaderEditor::Render_Code()
{
	static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
	float lineSize = max(10.0f, ImGui::GetContentRegionAvail().y / ImGui::GetTextLineHeight());
	ImGui::InputTextMultiline("##material_shader_editor_paramview_text", &m_pGUIMaterial->m_nslCodeDisplay, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * lineSize), flags);
}

void NXGUIMaterialShaderEditor::Render_Params(NXCustomMaterial* pMaterial)
{
	using namespace NXGUICommon;

	if (ImGui::TreeNodeEx("Parameters##material_shader_editor_parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
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

		ImGui::BeginChild("##material_shader_editor_custom_child");
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
		ImGui::TreePop();
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
