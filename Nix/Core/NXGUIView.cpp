#include "NXGUIView.h"
#include "NXTexture.h"
#include "NXInput.h"

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

		// 以viewRegion的约束范围为最大值，并始终保证RT比例正确。
		if (viewRegionX / viewRegionY > aspectRatio)
		{
			// 第一行和第三行用于计算居中，第二行用于计算约束。else同。
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

		ImVec2 viewPos(viewOffsetX, viewOffsetY + tabOffsetY);
		ImGui::SetCursorPos(viewPos);

		ImGui::Image((void*)m_pViewRT->GetSRV(), ImVec2(viewRegionX, viewRegionY));
		bool isHoveredOnView = ImGui::IsItemHovered();

		const auto& rectMin = ImGui::GetItemRectMin();
		const auto& rectMax = ImGui::GetItemRectMax();

		ImVec2 buttonSize(100, 30); 
		ImVec2 buttonPos(viewPos.x + 10.0f, viewPos.y + 10.0f);

		//ImGui::SetCursorPos(buttonPos);
		//if (ImGui::Button("View Size", buttonSize))
		//{
		//	ImGui::OpenPopup("##view_size_popup");
		//	isHoveredOnView &= !ImGui::IsItemHovered();
		//}

		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
		//if (ImGui::BeginPopup("##view_size_popup"))
		//{
		//	if (ImGui::MenuItem("1920 x 1080##view_size_popup_1080p"))
		//	{
		//		isHoveredOnView &= !ImGui::IsItemHovered();
		//	}
		//	if (ImGui::MenuItem("Auto##view_size_popup_auto"))
		//	{
		//		isHoveredOnView &= !ImGui::IsItemHovered();
		//	}
		//	if (ImGui::MenuItem("Custom##view_size_popup_custom"))
		//	{
		//		static int vs[2] = { 1280, 720 };
		//		ImGui::InputInt2("##view_size_popup_custom_input", vs);
		//		isHoveredOnView &= !ImGui::IsItemHovered();
		//	}
		//	ImGui::EndPopup();
		//}
		//ImGui::PopStyleVar();

		NXInput::GetInstance()->UpdateViewPortInput(isHoveredOnView, Vector4(rectMin.x, rectMin.y, rectMax.x, rectMax.y));
	}
	ImGui::End();
	ImGui::PopStyleVar();
}
