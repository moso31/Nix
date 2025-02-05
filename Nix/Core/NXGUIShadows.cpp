#include "BaseDefs/DearImGui.h"
#include "NXGUIShadows.h"

void NXGUIShadows::Render()
{
	ImGui::Begin("Shadows");

	NXShadowMapRenderer* pShadowMap = (NXShadowMapRenderer*)m_pRenderer->GetRenderGraph()->GetPass("ShadowMap");
	if (pShadowMap == nullptr)
	{
		ImGui::Text("ShadowMap pass not found");
		ImGui::End();
		return;
	}

	float shadowDistance = pShadowMap->GetShadowDistance();
	if (ImGui::DragFloat("Distance", &shadowDistance, 1.0f, 0.0f, 100000.0f))
	{
		pShadowMap->SetShadowDistance(shadowDistance);
	}

	float shadowExponent = pShadowMap->GetCascadeExponentScale();
	if (ImGui::DragFloat("Exponent", &shadowExponent, 0.01f, 1.0f, 10.0f))
	{
		pShadowMap->SetCascadeExponentScale(shadowExponent);
	}

	float cascadeTransition = pShadowMap->GetCascadeTransitionScale();
	if (ImGui::DragFloat("Transition scale", &cascadeTransition, 0.01f, 0.0f, 0.5f))
	{
		pShadowMap->SetCascadeTransitionScale(cascadeTransition);
	}

	float t = pShadowMap->m_test_transition;
	if (ImGui::DragFloat("Use Transition", &t, 1.0f, 0.0f, 1.0f))
	{
		pShadowMap->m_test_transition = t;
	}

	float depthBias = (float)pShadowMap->GetDepthBias();
	if (ImGui::DragFloat("Depth Bias", &depthBias, 1.0f, -100000.0f, 100000.0f))
	{
		pShadowMap->SetDepthBias((int)depthBias);
	}

	ImGui::End();
}
