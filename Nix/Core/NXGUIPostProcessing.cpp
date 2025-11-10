#include "BaseDefs/DearImGui.h"
#include "NXGUIPostProcessing.h"

NXGUIPostProcessing::NXGUIPostProcessing(Renderer* pRenderer) :
    m_pRenderer(pRenderer)
{
}

void NXGUIPostProcessing::Render()
{
	ImGui::Begin("Post Processing");

    bool bPostProcessingEnable = m_pRenderer->GetEnablePostProcessing();
    if (ImGui::Checkbox("Enable##post_processing", &bPostProcessingEnable))
    {
		m_pRenderer->SetEnablePostProcessing(bPostProcessingEnable);
    }

	ImGui::End();
}
