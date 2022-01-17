#pragma once
#include "Header.h"

class NXPBRLight;

class NXGUILights
{
public:
	NXGUILights(NXScene* pScene = nullptr);
	~NXGUILights() {}

	void Render();

private:
	NXScene* m_pCurrentScene;
	NXPBRLight* m_pCurrentLight;
	size_t m_currentItemIdx;
};