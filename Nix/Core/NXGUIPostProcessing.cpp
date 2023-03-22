#include "NXGUIPostProcessing.h"
#include "NXColorMappingRenderer.h"

NXGUIPostProcessing::NXGUIPostProcessing(NXColorMappingRenderer* pPostProcessingRenderer) :
    m_pPostProcessingRenderer(pPostProcessingRenderer)
{
}

void NXGUIPostProcessing::Render()
{
	ImGui::Begin("Post Processing");

    bool bPostProcessingEnable = m_pPostProcessingRenderer->GetEnable();
    if (ImGui::Checkbox("Enable##post_processing", &bPostProcessingEnable))
    {
        m_pPostProcessingRenderer->SetEnable(bPostProcessingEnable);
    }

	ImGui::End();
}
