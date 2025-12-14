#pragma once
#include <string>

class NXGUI;

class NXGUIWorkspace
{
public:
	NXGUIWorkspace() : m_bOpen(true), m_pGUI(nullptr) {}
	virtual ~NXGUIWorkspace() {}

	void Init(NXGUI* pGUI);
	void Render();

private:
	bool m_bOpen;
	std::string m_strVersion;
	std::string m_strMode;
	NXGUI* m_pGUI;
};
