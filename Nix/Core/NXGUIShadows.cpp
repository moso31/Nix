#include "BaseDefs/DearImGui.h"
#include "NXGUIShadows.h"
#include "NXGlobalBuffers.h"

void NXGUIShadows::Render()
{
	ImGui::Begin("Shadows");

	if (ImGui::DragFloat("Distance", &g_cbDataShadowTest.shadowDistance, 1.0f, 0.0f, 100000.0f)) {}

	float shadowExponent = m_pRenderer->GetShadowMapShadowExponent();
	if (ImGui::DragFloat("Exponent", &shadowExponent, 0.01f, 1.0f, 10.0f))
	{
		m_pRenderer->SetShadowMapShadowExponent(shadowExponent);
	}

	if (ImGui::DragFloat("Transition scale", &g_cbDataShadowTest.cascadeTransitionScale, 0.01f, 0.0f, 0.5f))
	{
	}

	if (ImGui::DragFloat("Use Transition", &g_cbDataShadowTest.test_transition, 1.0f, 0.0f, 1.0f))
	{
	}

	if (ImGui::DragInt("Depth Bias", &g_cbDataShadowTest.depthBias, 1.0f, -100000.0f, 100000.0f))
	{
	}

	ImGui::End();
}
