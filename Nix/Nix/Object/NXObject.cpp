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

shared_ptr<NXObject> NXObject::GetParent()
{
	return m_parent;
}

void NXObject::SetParent(shared_ptr<NXObject> pParent)
{
	m_parent = pParent;
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
