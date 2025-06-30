#include "BaseDefs/DearImGui.h"

#include "NXGUIInspector.h"
#include "NXGUICommandManager.h"
#include "NXGUITexture.h"
#include "NXGUIMaterial.h"
#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIDiffuseProfile.h"
#include "NXScene.h"
#include "NXGUITerrainSystem.h"

NXGUIInspector::NXGUIInspector()
{
}

void NXGUIInspector::InitGUI(NXScene* pScene, NXGUIMaterialShaderEditor* pMaterialShaderEditor, NXGUITerrainSystem* pTerrainSystem)
{
	m_pGUITexture = new NXGUITexture();
	m_pGUIMaterial = new NXGUIMaterial(pScene, pTerrainSystem);
	m_pGUIDiffuseProfile = new NXGUIDiffuseProfile();
	m_pGUIMaterialShaderEditor = pMaterialShaderEditor;
	m_pGUITerrainSystem = pTerrainSystem;
}

void NXGUIInspector::DoCommand(const NXGUICommand& cmd)
{
	switch (cmd.type)
	{
	case NXGUICmd_Inspector_SetIdx:
	{
		m_inspectorIndex = std::any_cast<NXGUIInspectorEnum>(cmd.args[0]);

		switch (m_inspectorIndex)
		{
			case NXGUIInspector_Texture:
			{
				auto path = std::any_cast<std::filesystem::path>(cmd.args[1]);
				m_pGUITexture->SetImage(path);
				break;
			}

			case NXGUIInspector_SubsurfaceProfile:
			{
				m_pGUIDiffuseProfile->SetDiffuseProfile(std::any_cast<std::filesystem::path>(cmd.args[1]));
				break;
			}
		}
		break;
	}
	case NXGUICmd_Inspector_OpenShaderEditor:
	{
		// 将参数和nsl代码从 当前GUI材质类 中同步到 MaterialShaderEditor
		m_pGUIMaterialShaderEditor->RequestSyncMaterialData();
		m_pGUIMaterialShaderEditor->RequestSyncMaterialCodes();

		// 并进行参数备份（revert功能要用）
		m_pGUIMaterialShaderEditor->RequestGenerateBackup();

		m_pGUIMaterialShaderEditor->Show();
		break;
	}
	case NXGUICmd_MSE_SetMaterial:
	{
		m_pGUIMaterialShaderEditor->SetMaterial(std::any_cast<NXCustomMaterial*>(cmd.args[0]));
		break;
	}
	case NXGUICmd_MSE_CompileSuccess:
	{
		m_pGUIMaterial->RequestSyncMaterialData();
		break;
	}
	default:
		break;
	}
}

void NXGUIInspector::Render()
{
	ImGui::Begin("Inspector");

	switch (m_inspectorIndex)
	{
	case NXGUIInspector_Texture:
		Render_Texture();
		break;
	case NXGUIInspector_Material:
		Render_Material();
		break;
	case NXGUIInspector_SubsurfaceProfile: 
		Render_SubsurfaceProfiler();
		break;
	default:
		ImGui::Text("Selected asset will be displayed in the Inspector.");
		break;
	}

	ImGui::End();
}

void NXGUIInspector::Release()
{
	SafeRelease(m_pGUITexture);
	SafeRelease(m_pGUIMaterial);
	SafeRelease(m_pGUIDiffuseProfile);
}

void NXGUIInspector::Render_Texture()
{
	m_pGUITexture->Render();
}

void NXGUIInspector::Render_Material()
{
	m_pGUIMaterial->Render();
}

void NXGUIInspector::Render_SubsurfaceProfiler()
{
	m_pGUIDiffuseProfile->Render();
}
