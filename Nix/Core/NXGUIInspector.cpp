#include "BaseDefs/DearImGui.h"

#include "NXGUIInspector.h"
#include "NXGUICommandManager.h"
#include "NXGUITexture.h"
#include "NXGUIMaterial.h"

NXGUIInspector::NXGUIInspector()
{
}

void NXGUIInspector::InitGUI()
{
	m_pGUITexture = new NXGUITexture();
}

void NXGUIInspector::DoCommand(const NXGUICommand& cmd)
{
	switch (cmd.type)
	{
	case NXGUICommandType::NXGUICmd_Inspector_SetIdx:
		m_inspectorIndex = std::any_cast<NXGUIInspectorEnum>(cmd.args[0]);
		break;
	case NXGUICommandType::NXGUICmd_Inspector_SetTexture:
		m_pGUITexture->SetImage(std::any_cast<std::filesystem::path>(cmd.args[0]));
		break;
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
	case NXGUIInspector_SubsurfaceProfiler: 
		Render_SubsurfaceProfiler();
		break;
	default: 
		break;
	}

	ImGui::End();
}

void NXGUIInspector::Release()
{
	SafeRelease(m_pGUITexture);
}

void NXGUIInspector::Render_Texture()
{
	ImGui::Text("Texture");
	m_pGUITexture->Render();
}

void NXGUIInspector::Render_SubsurfaceProfiler()
{
	ImGui::Text("SSS Diffuse Profiler");
}
