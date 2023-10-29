#pragma once
#include "BaseDefs/CppSTLFully.h"

enum NXGUIInspectorEnum
{
	NXGUIInspector_Nothing,
	NXGUIInspector_SubsurfaceProfiler,
};

class NXGUIInspector
{
public:
	NXGUIInspector();
	~NXGUIInspector() {}

	void Render();

private:
	void Render_SubsurfaceProfiler();

private:
	UINT m_inspectorIndex;
};