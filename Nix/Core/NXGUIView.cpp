#include "NXGUIView.h"
#include "NXTexture.h"
#include "NXFinalRenderer.h"
#include "NXEvent.h"

void NXGUIView::Init()
{
}

void NXGUIView::Render()
{
	ImGui::Begin("View");

	if (m_pFinalRenderer && m_pFinalRenderer->GetInputTexture())
	{
		ImGui::Image((void*)m_pFinalRenderer->GetInputTexture()->GetSRV(), ImGui::GetContentRegionAvail());
		g_bGuiOnViewportHover = ImGui::IsItemHovered();

		// Hover≤‚ ‘
		//g_bGuiOnViewportHover ? printf("Hovered\n") : printf("Not Hovered\n");
	}
	ImGui::End();
}
