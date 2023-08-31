#include "BaseDefs/DearImGui.h"

#include "NXGUIDebugLayer.h"
#include "NXDebugLayerRenderer.h"

void NXGUIDebugLayer::Render()
{
	ImGui::Begin("Debug Layer");
	bool bEnableDebugLayer = m_pDebugLayer->GetEnableDebugLayer();
	if (ImGui::Checkbox("Enable Debug Layer", &bEnableDebugLayer))
	{
		m_pDebugLayer->SetEnableDebugLayer(bEnableDebugLayer);
	}

	if (bEnableDebugLayer)
	{
		bool bEnableShadowMap = m_pDebugLayer->GetEnableShadowMapDebugLayer();
		if (ImGui::Checkbox("Cascade Shadow Map", &bEnableShadowMap))
		{
			m_pDebugLayer->SetEnableShadowMapDebugLayer(bEnableShadowMap);
		}

		float fZoomScale = m_pDebugLayer->GetShadowMapDebugLayerZoomScale();
		if (ImGui::DragFloat("Zoom Scale", &fZoomScale, 0.1f, 1.0f, 1024.0f))
		{
			m_pDebugLayer->SetShadowMapDebugLayerZoomScale(fZoomScale);
		}
	}

	ImGui::End();
}
