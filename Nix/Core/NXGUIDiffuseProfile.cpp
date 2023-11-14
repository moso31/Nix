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
	// 需要确保，在GUI调整Profile的效果能在View中立刻可见。
	// 所以需要一个指向 MaterialManager 的 m_sssProfilesMap 参数的指针 m_pShowProfile。
	// GUI 类 只负责改 m_pShowProfile 的值，MaterialManager 会自动维护即时更新的逻辑。
	m_pShowProfile = NXResourceManager::GetInstance()->GetMaterialManager()->GetOrAddSSSProfile(path);
}
