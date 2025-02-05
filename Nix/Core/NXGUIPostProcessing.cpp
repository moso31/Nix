#include "BaseDefs/DearImGui.h"
#include "NXGUIPostProcessing.h"

NXGUIPostProcessing::NXGUIPostProcessing(Renderer* pRenderer) :
    m_pRenderer(pRenderer)
{
}

void NXGUIPostProcessing::Render()
{
	ImGui::Begin("Post Processing");

	NXColorMappingRenderer* pPass = (NXColorMappingRenderer*)m_pRenderer->GetRenderGraph()->GetPass("PostProcessing");
	if (!pPass)
	{
		ImGui::Text("Debug Layer Renderer is not found.");
		ImGui::End();
		return;
	}

    bool bPostProcessingEnable = pPass->GetEnable();
    if (ImGui::Checkbox("Enable##post_processing", &bPostProcessingEnable))
    {
		pPass->SetEnable(bPostProcessingEnable);
    }

	ImGui::End();
}
