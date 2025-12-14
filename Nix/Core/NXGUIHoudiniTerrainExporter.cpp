#include "NXGUIHoudiniTerrainExporter.h"

NXGUIHoudiniTerrainExporter::NXGUIHoudiniTerrainExporter()
{
}

NXGUIHoudiniTerrainExporter::~NXGUIHoudiniTerrainExporter()
{
}

void NXGUIHoudiniTerrainExporter::Render()
{
	if (!m_bVisible)
		return;

	ImGui::SetNextWindowSize(ImVec2(500.0f, 300.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Houdini Terrain Exporter", &m_bVisible))
	{
		ImGui::Text("Houdini Terrain Exporter");
	}
	ImGui::End();
}
