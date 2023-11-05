#pragma once
#include "BaseDefs/NixCore.h"

enum NXGUIInspectorEnum
{
	NXGUIInspector_Nothing,
	NXGUIInspector_Texture,
	NXGUIInspector_Material,
	NXGUIInspector_SubsurfaceProfiler,
};

struct NXGUICommand;
class NXGUITexture;
class NXGUIMaterial;
class NXGUIInspector
{
public:
	NXGUIInspector();
	~NXGUIInspector() {}

	void InitGUI();

	void DoCommand(const NXGUICommand& cmd);
	void Render();

	void Release();

private:
	void Render_Texture();
	void Render_SubsurfaceProfiler();

private:
	NXGUIInspectorEnum m_inspectorIndex;
	NXGUITexture* m_pGUITexture;
	NXGUIMaterial* m_pGUIMaterial;
};
