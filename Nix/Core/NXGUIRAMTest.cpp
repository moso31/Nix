#include "NXGUIRAMTest.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"
#include "NXPrimitive.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"

NXGUIRAMTest::NXGUIRAMTest(NXScene* pScene):
	m_pScene(pScene),
	m_pTestMat(nullptr)
{
}

void NXGUIRAMTest::Init()
{
}

void NXGUIRAMTest::RenderBoxes()
{
}

void NXGUIRAMTest::Render()
{
	RenderBoxes();

	static int texId = 0;

	static int onceCreate = 10;
	ImGui::DragInt("Create texture count", &onceCreate, 1.0f, 0, 100);

	// set texture usage
	const char* strUsages[] = { "Default", "Immutable", "Dynamic", "Staging" };
	static int item = 0;
	static D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	if (ImGui::Combo("Texture type##Texture", &item, strUsages, IM_ARRAYSIZE(strUsages)))
	{
		usage = (D3D11_USAGE)item;
	}

	D3D11_CPU_ACCESS_FLAG cpuAccessFlag;
	D3D11_BIND_FLAG bindFlag;
	switch (usage)
	{
	case D3D11_USAGE_DEFAULT:
		cpuAccessFlag = (D3D11_CPU_ACCESS_FLAG)0;
		bindFlag = D3D11_BIND_SHADER_RESOURCE;
		break;
	case D3D11_USAGE_IMMUTABLE:
		cpuAccessFlag = (D3D11_CPU_ACCESS_FLAG)0;
		bindFlag = D3D11_BIND_SHADER_RESOURCE;
		break;
	case D3D11_USAGE_DYNAMIC:
		cpuAccessFlag = D3D11_CPU_ACCESS_WRITE;
		bindFlag = D3D11_BIND_SHADER_RESOURCE;
		break;
	case D3D11_USAGE_STAGING:
		cpuAccessFlag = D3D11_CPU_ACCESS_FLAG(D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ);
		bindFlag = (D3D11_BIND_FLAG)0;
		break;
	}

	static float fPositiveOffset = 0.0f;
	ImGui::DragFloat("FrontOffset", &fPositiveOffset);

	static float fPositiveArea = 100.0f;
	ImGui::DragFloat("FrontArea", &fPositiveArea);

	if (ImGui::ButtonEx("Create Texture2D", ImVec2(200, 50)))
	{
		for (int i = 0; i < onceCreate; i++)
		{
			auto pTexture2D = m_pTextures.emplace_back(new NXTexture2D());
			pTexture2D->CreateRaw("D:\\NixAssets\\hex-stones1\\albedo.png", usage, cpuAccessFlag, bindFlag);
			pTexture2D->AddSRV();
		}
	}

	ImGui::SameLine();
	if (ImGui::ButtonEx("Remove Textures", ImVec2(200, 50)))
	{
		for (auto pTex : m_pTextures)
		{
			SafeRelease(pTex);
		}

		m_pTextures.clear();
	}

	if (ImGui::ButtonEx("Create Boxes", ImVec2(200, 50)))
	{
		auto pBox = NXResourceManager::GetInstance()->GetMeshManager()->CreateBox("Box", 1.0f, 1.0f, 1.0f, Vector3(0.0f, 0.0f, 10.0f));

		if (pBox)
		{
			auto pDefaultMaterial = NXResourceManager::GetInstance()->GetMaterialManager()->GetDefaultMaterial();
			NXResourceManager::GetInstance()->GetMeshManager()->BindMaterial(pBox, pDefaultMaterial);

			m_pTestBoxes.push_back(pBox);
		}
	}

	ImGui::SameLine();
	if (ImGui::ButtonEx("Delete Boxes", ImVec2(200, 50)))
	{
		for (auto pBox : m_pTestBoxes)
		{
			NXResourceManager::GetInstance()->GetMeshManager()->RemoveRenderableObject(pBox);
		}
		m_pTestBoxes.clear();
	}

	if (ImGui::ButtonEx("Generate Materials", ImVec2(200, 50)))
	{
		m_pTestMat = NXResourceManager::GetInstance()->GetMaterialManager()->CreateTestMaterial("hello");
		auto pWhiteTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
		for (int i = 0; i < 100; i++)
		{
			m_pTestMat->SetTexture(i, i < m_pTextures.size() ? m_pTextures[i] : pWhiteTex);
		}

		if (m_pTestMat)
		{
			for (auto& pBox : m_pTestBoxes)
			{
				NXResourceManager::GetInstance()->GetMeshManager()->BindMaterial(pBox, m_pTestMat);
			}
		}
	}
}

void NXGUIRAMTest::Release()
{
	for (auto pTex : m_pTextures)
		SafeRelease(pTex);
}

