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
		m_pRenderer->NotifyRebuildRenderGraph();
	}

	if (bEnableDebugLayer)
	{
		bool bEnableShadowMap = m_pRenderer->GetEnableShadowMapDebugLayer();
		if (ImGui::Checkbox("Cascade Shadow Map", &bEnableShadowMap))
		{
			m_pRenderer->SetEnableShadowMapDebugLayer(bEnableShadowMap);
		}

		float fZoomScale = m_pRenderer->GetShadowMapDebugLayerZoomScale();
		if (ImGui::DragFloat("Zoom Scale", &fZoomScale, 0.1f, 1.0f, 1024.0f))
		{
			m_pRenderer->SetShadowMapDebugLayerZoomScale(fZoomScale);
		}
	}

	ImGui::End();
}
