#include "NXGUIView.h"
#include "NXTexture.h"
#include "NXFinalRenderer.h"
#include "NXInput.h"

void NXGUIView::Init()
{
}

void NXGUIView::Render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
	ImGui::Begin("View");

	if (m_pFinalRenderer && m_pFinalRenderer->GetInputTexture())
	{
		ImGui::Image((void*)m_pFinalRenderer->GetInputTexture()->GetSRV(), ImGui::GetContentRegionAvail());
		const auto& rectMin = ImGui::GetItemRectMin();
		const auto& rectMax = ImGui::GetItemRectMax();
		NXInput::GetInstance()->UpdateViewPortInput(ImGui::IsItemHovered(), Vector4(rectMin.x, rectMin.y, rectMax.x, rectMax.y));

		// Hover≤‚ ‘
		//g_bGuiOnViewportHover ? printf("Hovered\n") : printf("Not Hovered\n");
	}
	ImGui::End();
	ImGui::PopStyleVar();
}
