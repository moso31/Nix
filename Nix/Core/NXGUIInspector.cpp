#include "BaseDefs/DearImGui.h"

#include "NXGUIInspector.h"

NXGUIInspector::NXGUIInspector()
{
}

void NXGUIInspector::Render()
{
	ImGui::Begin("Inspector");

	switch (m_inspectorIndex)
	{
	case NXGUIInspector_SubsurfaceProfiler: 
		Render_SubsurfaceProfiler();
		break;
	default: 
		break;
	}

	ImGui::End();
}

void NXGUIInspector::Render_SubsurfaceProfiler()
{
}
