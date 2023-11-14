#include "BaseDefs/DearImGui.h"
#include "NXGUIDiffuseProfile.h"
#include "NXResourceManager.h"
#include "NXSSSDiffuseProfile.h"

NXGUIDiffuseProfile::NXGUIDiffuseProfile()
{
}

void NXGUIDiffuseProfile::Render()
{
	Vector3 val = m_pShowProfile->GetScatter();
	if (ImGui::ColorEdit3("", val))
	{
		m_pShowProfile->SetScatter(val);
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
