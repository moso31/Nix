#include "BaseDefs/DearImGui.h"

#include "NXGUIInspector.h"
#include "NXGUICommandManager.h"
#include "NXGUITexture.h"
#include "NXGUIMaterial.h"
#include "NXGUIMaterialShaderEditor.h"
#include "NXScene.h"

NXGUIInspector::NXGUIInspector()
{
}

void NXGUIInspector::InitGUI(NXScene* pScene, NXGUIMaterialShaderEditor* pMaterialShaderEditor)
{
	m_pGUITexture = new NXGUITexture();
	m_pGUIMaterial = new NXGUIMaterial(pScene);
	m_pGUIMaterialShaderEditor = pMaterialShaderEditor;
}

void NXGUIInspector::DoCommand(const NXGUICommand& cmd)
{
	switch (cmd.type)
	{
	case NXGUICmd_Inspector_SetIdx:
		m_inspectorIndex = std::any_cast<NXGUIInspectorEnum>(cmd.args[0]);
		break;
	case NXGUICmd_Inspector_SetTexture:
		m_pGUITexture->SetImage(std::any_cast<std::filesystem::path>(cmd.args[0]));
		break;
	case NXGUICmd_Inspector_OpenShaderEditor:
	{
		// ��������nsl����� ��ǰGUI������ ��ͬ���� MaterialShaderEditor
		m_pGUIMaterialShaderEditor->RequestSyncMaterialData();
		m_pGUIMaterialShaderEditor->RequestSyncMaterialCodes();

		// �����в������ݣ�revert����Ҫ�ã�
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
	case NXGUIInspector_SubsurfaceProfiler: 
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
	ImGui::Text("SSS Diffuse Profiler");
}
