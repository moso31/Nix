#include "NXObject.h"
#include "NXScript.h"

NXObject::~NXObject()
{
}

string NXObject::GetName()
{
	return m_name;
}

void NXObject::SetName(string name)
{
	m_name = name;
}

void NXObject::AddScript(const shared_ptr<NXScript> &script)
{
	script->SetObject(shared_from_this());
	m_scripts.push_back(script);
}

vector<shared_ptr<NXScript>> NXObject::GetScripts()
{
	return m_scripts;
}

void NXObject::Update()
{
	for (auto it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		(*it)->Update();
	}
}

void NXObject::Release()
{
	for (auto it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		(*it).reset();
	}
}
