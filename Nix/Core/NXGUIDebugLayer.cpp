#include "BaseDefs/DearImGui.h"

#include "NXGUIDebugLayer.h"
#include "NXDebugLayerRenderer.h"

void NXGUIDebugLayer::Render()
{
	ImGui::Begin("Debug Layer");

	NXDebugLayerRenderer* pDebugLayer = (NXDebugLayerRenderer*)m_pRenderer->GetRenderGraph()->GetRenderPass("DebugLayer");
	if (!pDebugLayer)
	{
		ImGui::Text("Debug Layer Renderer is not found.");
		ImGui::End();
		return;
	}

	bool bEnableDebugLayer = m_pRenderer->GetEnableDebugLayer();
	if (ImGui::Checkbox("Enable Debug Layer", &bEnableDebugLayer))
	{
		m_pRenderer->SetEnableDebugLayer(bEnableDebugLayer);
	}

	if (bEnableDebugLayer)
	{
		bool bEnableShadowMap = pDebugLayer->GetEnableShadowMapDebugLayer();
		if (ImGui::Checkbox("Cascade Shadow Map", &bEnableShadowMap))
		{
			pDebugLayer->SetEnableShadowMapDebugLayer(bEnableShadowMap);
		}

		float fZoomScale = pDebugLayer->GetShadowMapDebugLayerZoomScale();
		if (ImGui::DragFloat("Zoom Scale", &fZoomScale, 0.1f, 1.0f, 1024.0f))
		{
			pDebugLayer->SetShadowMapDebugLayerZoomScale(fZoomScale);
		}
	}

	ImGui::End();
}
