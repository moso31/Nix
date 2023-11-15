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

	float radiusM = m_pShowProfile->GetRadius();
	{
		float radiusCM = radiusM * 100.0f;
		if (ImGui::DragFloat("Radius (cm)", &radiusCM, 0.01f))
		{
			m_pShowProfile->SetRadius(radiusCM * 0.01f);
		}
	}

	Vector3 scatter = m_pShowProfile->GetScatter();
	if (ImGui::ColorEdit3("Scatter Color", scatter))
	{
		m_pShowProfile->SetScatter(scatter);
	}

	float scatterStrength = m_pShowProfile->GetScatterStrength();
	if (ImGui::DragFloat("Scatter Strength", &scatterStrength, 0.01f))
	{
		m_pShowProfile->SetScatterStrength(scatterStrength);
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
