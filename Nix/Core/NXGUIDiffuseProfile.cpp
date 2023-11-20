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
	// ��Ҫȷ������GUI����Profile��Ч������View�����̿ɼ���
	// ������Ҫһ��ָ�� MaterialManager �� m_sssProfilesMap ������ָ�� m_pShowProfile��
	// GUI �� ֻ����� m_pShowProfile ��ֵ��MaterialManager ���Զ�ά����ʱ���µ��߼���
	m_pShowProfile = NXResourceManager::GetInstance()->GetMaterialManager()->GetOrAddSSSProfile(path);
}
