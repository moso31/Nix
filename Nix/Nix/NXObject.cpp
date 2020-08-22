#include "NXObject.h"
#include "NXScript.h"

NXObject::NXObject() :
	m_parent(nullptr)
{
}

NXObject::~NXObject()
{
}

std::string NXObject::GetName()
{
	return m_name;
}

void NXObject::SetName(std::string name)
{
	m_name = name;
}

void NXObject::AddScript(const std::shared_ptr<NXScript> &script)
{
	script->SetObject(shared_from_this());
	m_scripts.push_back(script);
}

std::vector<std::shared_ptr<NXScript>> NXObject::GetScripts()
{
	return m_scripts;
}

std::shared_ptr<NXObject> NXObject::GetParent()
{
	return m_parent;
}

void NXObject::SetParent(std::shared_ptr<NXObject> pParent)
{
	if (m_parent)
		m_parent->RemoveChild(shared_from_this());

	// 不考虑SetParent为nullptr的情况，因为场景中始终有一个pRootObject。
	m_parent = pParent;
	pParent->m_childs.push_back(shared_from_this());
}

size_t NXObject::GetChildCount()
{
	return m_childs.size();
}

std::list<std::shared_ptr<NXObject>> NXObject::GetChilds()
{
	return m_childs;
}

void NXObject::RemoveChild(const std::shared_ptr<NXObject>& pObject)
{
	m_childs.remove(pObject);
}

bool NXObject::IsChild(std::shared_ptr<NXObject> pObject)
{
	auto pParent = pObject->m_parent;
	while (pParent)
	{
		if (pParent == shared_from_this()) return true;
		pParent = pParent->m_parent;
	}
	return false;
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