#include "BaseDefs/DearImGui.h"

#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXTexture.h"

NXGUITexture::NXGUITexture() :
	m_pTexImage(nullptr)
{
}

void NXGUITexture::Render()
{
	ImGui::Begin("Texture");

	if (!m_pTexImage)
	{ 
		ImGui::End();
		return;
	}

	float fTexSize = ImGui::GetContentRegionAvail().x * 0.7f;
	ImGui::Image(ImTextureID(m_pTexImage->GetSRV()), ImVec2(fTexSize, fTexSize));

	ImGui::Checkbox("Generate mip map##Texture", &m_texData.m_bGenerateMipMap);
	ImGui::Checkbox("Invert normal Y##Texture", &m_texData.m_bInvertNormalY);

	const char* strTextureTypes[] = { "Raw", "sRGB", "Linear", "Normal Map" };
	int nTexType = (int)m_texData.m_textureType;
	if (ImGui::Combo("Texture type##Texture", &nTexType, strTextureTypes, IM_ARRAYSIZE(strTextureTypes)))
	{
		m_texData.m_textureType = (NXTextureType)(nTexType);
	}

	if (ImGui::Button("Apply##Texture"))
	{
		m_pTexImage->SetSerializationData(m_texData);

		// ����NXInfo�ļ�
		m_pTexImage->Serialize();
		m_pTexImage->MarkReload();
	}
	ImGui::End();
}

void NXGUITexture::Release()
{
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	// ���·�����Ϸ�������ʾ������Ϣ
	if (path.empty())
	{
		m_pTexImage = nullptr;
		return;
	}

	if (m_pTexImage)
	{
		// ���·����ͬ���������¼���
		if (path == m_pTexImage->GetFilePath())
			return;

		// ·����ͬ���ͷžɵ��������Ҳ���ʾ������Ϣ
		m_pTexImage->RemoveRef();
	}

	// �����֮ǰ��·����ͬ���ͼ����µ�����
	m_pTexImage = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("NXGUITexture Preview Image", path);
	m_texData = m_pTexImage->GetSerializationData();
}
