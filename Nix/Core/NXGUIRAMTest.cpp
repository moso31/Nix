#include "NXGUIRAMTest.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"
#include "NXPrimitive.h"
#include "ShaderComplier.h"
#include "NXRenderStates.h"
#include "GlobalBufferManager.h"

NXGUIRAMTest::NXGUIRAMTest(NXScene* pScene):
	m_pScene(pScene)
{
}

void NXGUIRAMTest::Init()
{
}

void NXGUIRAMTest::PreRender()
{
	g_pContext->ClearState();
	g_pContext->Flush();
}

void NXGUIRAMTest::Render()
{
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

	static float fPositiveOffset = 10.0f;
	ImGui::DragFloat("Offset", &fPositiveOffset);

	static float fPositiveArea = 100.0f;
	ImGui::DragFloat("Area", &fPositiveArea);

	if (ImGui::ButtonEx("Create at Front"))
	{
		CreateBoxes(true, fPositiveOffset, fPositiveArea, onceCreate, usage, cpuAccessFlag, bindFlag);
	}
	ImGui::SameLine();
	if (ImGui::ButtonEx("Create at Back"))
	{
		CreateBoxes(false, fPositiveOffset, fPositiveArea, onceCreate, usage, cpuAccessFlag, bindFlag);
	}

	if (ImGui::ButtonEx("Clear Front"))
	{
		ClearBoxes(true);
	}

	if (ImGui::ButtonEx("Clear Back"))
	{
		ClearBoxes(false);
	}
}

void NXGUIRAMTest::Release()
{
	for (auto pTex : m_pTextures)
		SafeRelease(pTex);
	for (auto pTex : m_pTexturesBack)
		SafeRelease(pTex);
}

void NXGUIRAMTest::CreateBoxes(bool isFront, const float fOffset, const float fArea, const int nAmount, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpuAccessFlag, D3D11_BIND_FLAG bindFlag)
{
	int nHeight = (int)sqrtf((float)nAmount);
	int nWidth = (nAmount + nHeight - 1) / nHeight;

	float fStepWidth = fArea / (float)nWidth;
	float fStepHeight = fArea / (float)nHeight;

	int n = 0;
	for (int i = 0; i < nWidth; i++)
	{
		for (int j = 0; j < nHeight; j++)
		{
			if (n++ == nAmount) 
				break;

			auto pTexture2D = isFront ? m_pTextures.emplace_back(new NXTexture2D()) : m_pTexturesBack.emplace_back(new NXTexture2D());
			pTexture2D->CreateRaw("D:\\NixAssets\\hex-stones1\\albedo.png", usage, cpuAccessFlag, bindFlag);
			pTexture2D->AddSRV();

			Vector3 Pos(-fArea * 0.5f + fStepWidth * i, -fArea * 0.5f + fStepHeight * j, isFront ? fOffset : -fOffset);
			auto pBox = NXResourceManager::GetInstance()->GetMeshManager()->CreateBox("Box", 1.0f, 1.0f, 1.0f, Pos);
			if (pBox)
			{
				auto pMat = NXResourceManager::GetInstance()->GetMaterialManager()->CreateTestMaterial(pTexture2D, isFront);
				if (pMat)
				{
					NXResourceManager::GetInstance()->GetMeshManager()->BindMaterial(pBox, pMat);

					isFront ? m_pBoxes.push_back(pBox) : m_pBoxesBack.push_back(pBox);
				}
			}
		}
	}
}

void NXGUIRAMTest::ClearBoxes(bool isFront)
{
	if (isFront)
	{
		for (auto pBox : m_pBoxes) NXResourceManager::GetInstance()->GetMeshManager()->RemoveRenderableObject(pBox);
		m_pBoxes.clear();
	}
	else
	{
		for (auto pBox : m_pBoxesBack) NXResourceManager::GetInstance()->GetMeshManager()->RemoveRenderableObject(pBox);
		m_pBoxesBack.clear();
	}

	NXResourceManager::GetInstance()->GetMaterialManager()->RemoveTestMaterials(isFront);

	if (isFront)
	{
		for (auto pTex : m_pTextures) SafeRelease(pTex);
		m_pTextures.clear();
	}
	else
	{
		for (auto pTex : m_pTexturesBack) SafeRelease(pTex);
		m_pTexturesBack.clear();
	}

	m_bForceFlush = true;
}

