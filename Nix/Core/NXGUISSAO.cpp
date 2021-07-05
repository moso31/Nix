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

	float SSAORadius = m_pSSAO->GetRadius();
	if (ImGui::DragFloat("Radius", &SSAORadius, 0.01f, 0.05f, 5.0f))
	{
		m_pSSAO->SetRadius(SSAORadius);
	}

	ImGui::End();
}
