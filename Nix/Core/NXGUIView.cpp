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
	ImGui::Begin("View");

	if (m_pViewRT && m_pViewRT->GetSRV())
	{
		ImGui::Image((void*)m_pViewRT->GetSRV(), ImGui::GetContentRegionAvail());
		const auto& rectMin = ImGui::GetItemRectMin();
		const auto& rectMax = ImGui::GetItemRectMax();
		NXInput::GetInstance()->UpdateViewPortInput(ImGui::IsItemHovered(), Vector4(rectMin.x, rectMin.y, rectMax.x, rectMax.y));
	}
	ImGui::End();
	ImGui::PopStyleVar();
}
