#include "NXGUIView.h"
#include "NXTexture.h"
#include "NXInput.h"
#include "App.h"

void NXGUIView::SetViewRT(NXTexture2D* pTex)
{
	m_pViewRT = pTex;
}

void NXGUIView::Render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
	ImGui::Begin("View", false, ImGuiWindowFlags_NoScrollbar);

	if (m_pViewRT && m_pViewRT->GetSRV())
	{
		float windowHeight = ImGui::GetWindowHeight();

		float aspectRatio = (float)m_pViewRT->GetWidth() / (float)m_pViewRT->GetHeight();
		float viewRegionX = ImGui::GetContentRegionAvail().x;
		float viewRegionY = ImGui::GetContentRegionAvail().y;

		// Y向下偏移一点，否则选项卡会遮挡
		float tabOffsetY = windowHeight - viewRegionY - 1;
		float viewOffsetX = 0.0f;
		float viewOffsetY = 0.0f;

		if (m_viewMode != ViewMode::Auto)
		{
			// 若不是 Auto 模式，需要
			// 以viewRegion的约束范围为最大值，并始终保证RT比例正确。
			if (viewRegionX / viewRegionY > aspectRatio)
			{
				// 第一行和第三行用于计算居中偏移量，第二行用于计算约束区大小。else同。
				viewOffsetX = viewRegionX;
				viewRegionX = viewRegionY * aspectRatio;
				viewOffsetX = (viewOffsetX - viewRegionX) / 2;
			}
			else
			{
				viewOffsetY = viewRegionY;
				viewRegionY = viewRegionX / aspectRatio;
				viewOffsetY = (viewOffsetY - viewRegionY) / 2;
			}
		}

		ImVec2 viewPos(viewOffsetX, viewOffsetY + tabOffsetY);
		ImGui::SetCursorPos(viewPos);

		ImGui::Image((void*)m_pViewRT->GetSRV(), ImVec2(viewRegionX, viewRegionY));
		bool isHoveredOnView = ImGui::IsItemHovered();

		// 点击右键时也使当前窗口获得焦点，避免其它窗口（比如CodeEditor）输入收到WASD等快捷键影响
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) ImGui::SetWindowFocus(); 

		const auto& rectMin = ImGui::GetItemRectMin();
		const auto& rectMax = ImGui::GetItemRectMax();

		ImVec2 buttonPos(viewPos.x + 5.0f, viewPos.y + 5.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 0.9f));
		ImGui::SetCursorPos(buttonPos);
		std::string strBtnName = "View Size: " + GetViewModeString();
		if (ImGui::Button(strBtnName.c_str()))
		{
			ImGui::OpenPopup("##view_size_popup");
		}
		isHoveredOnView &= !ImGui::IsItemHovered();
		ImGui::PopStyleColor();

		static int vs[2] = { 1280, 720 };
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
		if (ImGui::BeginPopup("##view_size_popup"))
		{
			if (ImGui::MenuItem("1920x1080##view_size_popup_1080p"))
			{
				m_viewMode = ViewMode::Fix_1920x1080;
			}
			isHoveredOnView &= !ImGui::IsItemHovered();

			std::string strAutoText = "Auto(" + std::to_string((int)viewRegionX) + "x" + std::to_string((int)viewRegionY) + ")##view_size_popup_auto";
			if (ImGui::MenuItem(strAutoText.c_str()))
			{
				m_viewMode = ViewMode::Auto;
			}
			isHoveredOnView &= !ImGui::IsItemHovered();

			//if (ImGui::MenuItem("Custom##view_size_popup_custom"))
			{
				ImGui::Text("Custom");
				isHoveredOnView &= !ImGui::IsItemHovered();

				ImGui::SameLine();

				ImGui::PushItemWidth(100.0f);
				ImGui::InputInt2("##view_size_popup_custom_input", vs);
				isHoveredOnView &= !ImGui::IsItemHovered();
				ImGui::PopItemWidth();

				ImGui::SameLine();
				if (ImGui::Button("Apply"))
				{
					m_viewMode = ViewMode::Custom;
					ImGui::CloseCurrentPopup();
				}
				isHoveredOnView &= !ImGui::IsItemHovered();
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		switch (m_viewMode)
		{
		case ViewMode::Auto:
			m_viewRTSize = { viewRegionX, viewRegionY };
			break;
		case ViewMode::Custom:
			m_viewRTSize = { (float)vs[0], (float)vs[1] };
			break;
		case ViewMode::Fix_1920x1080:
			m_viewRTSize = { 1920.0f, 1080.0f };
			break;
		default:
			break;
		}

		g_app->SetViewSize(m_viewRTSize);

		NXInput::GetInstance()->UpdateViewPortInput(isHoveredOnView, Vector4(rectMin.x, rectMin.y, rectMax.x, rectMax.y));
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

const std::string NXGUIView::GetViewModeString() const
{
	switch (m_viewMode)
	{
	case NXGUIView::ViewMode::Auto:
		return "Auto";
		break;
	case NXGUIView::ViewMode::Custom:
		return "Custom";
		break;
	case NXGUIView::ViewMode::Fix_1920x1080:
		return "Fixed";
		break;
	default:
		return "*Unknown*";
		break;
	}
}
