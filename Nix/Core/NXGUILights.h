#pragma once

class NXScene;
class NXPBRLight;
class NXGUILights
{
public:
	NXGUILights(NXScene* pScene = nullptr);
	virtual ~NXGUILights() {}

	void Render();

private:
	NXScene* m_pCurrentScene;
	NXPBRLight* m_pCurrentLight;

	size_t m_currentItemIdx;
};