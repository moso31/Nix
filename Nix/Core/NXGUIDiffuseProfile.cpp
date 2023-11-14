#include "NXGUIDiffuseProfile.h"
#include "NXResourceManager.h"

NXGUIDiffuseProfile::NXGUIDiffuseProfile()
{
}

void NXGUIDiffuseProfile::Render()
{
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
