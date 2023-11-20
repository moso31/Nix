#include "BaseDefs/DearImGui.h"
#include "NXGUIDiffuseProfile.h"
#include "NXResourceManager.h"
#include "NXSSSDiffuseProfile.h"

NXGUIDiffuseProfile::NXGUIDiffuseProfile()
{
}

void NXGUIDiffuseProfile::Render()
{
	if (m_pShowProfile.IsNull())
		return;

	Vector3 scatter = m_pShowProfile->GetScatter();
	if (ImGui::ColorEdit3("Scatter Color", scatter))
	{
		m_pShowProfile->SetScatter(scatter);
	}

	float scatterDistance = m_pShowProfile->GetScatterDistance();
	if (ImGui::DragFloat("Scatter Distance", &scatterDistance, 0.01f))
	{
		m_pShowProfile->SetScatterDistance(scatterDistance);
	}

	Vector3 transmit = m_pShowProfile->GetTransmit();
	if (ImGui::ColorEdit3("Transmit Color", transmit))
	{
		m_pShowProfile->SetTransmit(transmit);
	}

	float transmitStrength = m_pShowProfile->GetTransmitStrength();
	if (ImGui::DragFloat("Transmit Strength", &transmitStrength, 0.01f))
	{
		m_pShowProfile->SetTransmitStrength(transmitStrength);
	}
}

void NXGUIDiffuseProfile::Release()
{
}

void NXGUIDiffuseProfile::SetDiffuseProfile(const std::filesystem::path& path)
{
	// 需要确保，在GUI调整Profile的效果能在View中立刻可见。
	// 所以需要一个指向 MaterialManager 的 m_sssProfilesMap 参数的指针 m_pShowProfile。
	// GUI 类 只负责改 m_pShowProfile 的值，MaterialManager 会自动维护即时更新的逻辑。
	m_pShowProfile = NXResourceManager::GetInstance()->GetMaterialManager()->GetOrAddSSSProfile(path);
}
