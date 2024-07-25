#include "NXScriptable.h"
#include "NXScript.h"

NXScriptable::~NXScriptable()
{
	Destroy();
}

void NXScriptable::AddScript(NXScript* script)
{
	script->SetObject(this);
	m_scripts.push_back(script);
}

std::vector<NXScript*> NXScriptable::GetScripts()
{
	return m_scripts;
}

void NXScriptable::UpdateScript()
{
	for (auto script : m_scripts)
	{
		script->Update();
	}
}

void NXScriptable::Destroy()
{
	for (auto script : m_scripts)
	{
		delete script; 
		script = nullptr;
	}
}
