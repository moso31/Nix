#pragma once
#include <vector>

class NXScript;
class NXScriptable
{
public:
	NXScriptable() {}
	virtual ~NXScriptable();

	void AddScript(NXScript* script);
	std::vector<NXScript*> GetScripts();

	virtual void UpdateScript();

	void Destroy();

protected:
	std::vector<NXScript*> m_scripts;
};
