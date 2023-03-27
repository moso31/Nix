#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"

NXGUITexture::NXGUITexture() :
	m_pTexInfo(nullptr),
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

	ImGui::Image(ImTextureID(m_pTexImage->GetSRV()), ImVec2(200.0f, 200.0f));

	if (m_pTexInfo)
	{
		bool bGenerateMipMap = m_pTexInfo->bGenerateMipMap;
		if (ImGui::Checkbox("Generate mip map##Texture", &bGenerateMipMap))
		{
			m_pTexInfo->bGenerateMipMap = bGenerateMipMap;
		}

		bool bInvertNormalY = m_pTexInfo->bInvertNormalY;
		if (ImGui::Checkbox("Invert normal Y##Texture", &bInvertNormalY))
		{
			m_pTexInfo->bInvertNormalY = bInvertNormalY;
		}

		int nTexType = m_pTexInfo->nTexType;
		static const char* items[] = { "Default", "Normal map" };
		if (ImGui::Combo("Texture type##Texture", &nTexType, items, IM_ARRAYSIZE(items)))
		{
			m_pTexInfo->nTexType = nTexType;
		}

		if (m_pTexInfo->nTexType == 0)
		{
			bool bSRGB = m_pTexInfo->bSRGB;
			if (ImGui::Checkbox("sRGB##Texture", &bSRGB))
			{
				m_pTexInfo->bSRGB = bSRGB;
			}
		}

		if (ImGui::Button("Apply##Texture"))
		{
			// ����NXInfo�ļ�
			NXResourceManager::GetInstance()->SaveTextureInfo(m_pTexInfo, m_pTexImage->GetFilePath());
			m_pTexImage->MarkReload();

			SetImage(m_pTexImage->GetFilePath());
		}
	}

	ImGui::End();
}

void NXGUITexture::Release()
{
	if (m_pTexImage) m_pTexImage->RemoveRef();
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	// ���·�����Ϸ�������ʾ������Ϣ
	if (path.empty())
	{
		m_pTexInfo = nullptr;
		return;
	}

	if (m_pTexImage)
	{
		// ���·����ͬ���������¼���
		if (path == m_pTexImage->GetFilePath())
		{
			m_pTexInfo = m_pTexImage->GetTextureNXInfo();
			return;
		}

		// ·����ͬ���ͷžɵ��������Ҳ���ʾ������Ϣ
		m_pTexImage->RemoveRef();
		m_pTexInfo = nullptr;
	}

	// �����֮ǰ��·����ͬ���ͼ����µ�����
	m_pTexImage = NXResourceManager::GetInstance()->CreateTexture2D("NXGUITexture Preview Image", path);
	if (m_pTexImage)
		m_pTexInfo = m_pTexImage->GetTextureNXInfo();
}
