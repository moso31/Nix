#include "NXGUIShadows.h"
#include "NXShadowMapRenderer.h"

void NXGUIShadows::Render()
{
	ImGui::Begin("Shadows");

	float shadowDistance = m_pShadowMap->GetShadowDistance();
	if (ImGui::DragFloat("Distance", &shadowDistance, 1.0f, 0.0f, 100000.0f))
	{
		m_pShadowMap->SetShadowDistance(shadowDistance);
	}

	float shadowExponent = m_pShadowMap->GetCascadeExponentScale();
	if (ImGui::DragFloat("Exponent", &shadowExponent, 0.01f, 1.0f, 10.0f))
	{
		m_pShadowMap->SetCascadeExponentScale(shadowExponent);
	}

	float cascadeTransition = m_pShadowMap->GetCascadeTransitionScale();
	if (ImGui::DragFloat("Transition scale", &cascadeTransition, 0.01f, 0.0f, 0.5f))
	{
		m_pShadowMap->SetCascadeTransitionScale(cascadeTransition);
	}

	float t = m_pShadowMap->m_test_transition;
	if (ImGui::DragFloat("Use Transition", &t, 1.0f, 0.0f, 1.0f))
	{
		m_pShadowMap->m_test_transition = t;
	}

	float depthBias = m_pShadowMap->GetDepthBias();
	if (ImGui::DragFloat("Depth Bias", &depthBias, 1.0f, -100000.0f, 100000.0f))
	{
		m_pShadowMap->SetDepthBias(depthBias);
	}

	ImGui::End();
}
