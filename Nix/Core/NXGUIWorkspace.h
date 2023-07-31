#pragma once
#include "Header.h"

class NXGUIWorkspace
{
public:
	NXGUIWorkspace() : m_bOpen(true) {}
	~NXGUIWorkspace() {}

	void Init();
	void Render();

private:
	bool m_bOpen;
	std::string m_strVersion;
	std::string m_strMode;
};
