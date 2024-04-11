#include "BaseDefs/DearImGui.h"

#include "NXGUISSAO.h"
#include "NXSimpleSSAO.h"

NXGUISSAO::NXGUISSAO(NXSimpleSSAO* pSSAO) :
	m_pSSAO(pSSAO),
	m_bDirty(false)
{
}

void NXGUISSAO::Render()
{
	m_bDirty = false;
	ImGui::Begin("SSAO");

	//float SSAORadius = m_pSSAO->GetRadius();
	//if (ImGui::DragFloat("Radius", &SSAORadius, 0.01f, 0.05f, 5.0f))
	//{
	//	m_pSSAO->SetRadius(SSAORadius);
	//}

	//float SSAOBias = m_pSSAO->GetBias();
	//if (ImGui::DragFloat("Bias", &SSAOBias, 0.01f, 0.0f, 1.0f))
	//{
	//	m_pSSAO->SetBias(SSAOBias);
	//}

	//float SSAODirectLightingStrength = m_pSSAO->GetDirectLightingStrength();
	//if (ImGui::DragFloat("Direct Lighting Strength", &SSAODirectLightingStrength, 0.01f, 0.0f, 1.0f))
	//{
	//	m_pSSAO->SetDirectLightingStrength(SSAODirectLightingStrength);
	//}

	ImGui::End();
}
