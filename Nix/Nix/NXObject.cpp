#include "NXObject.h"
#include "NXScript.h"

NXObject::NXObject() :
	m_type(NXType::eNone),
	m_parent(nullptr)
{
}

NXObject::~NXObject()
{
}

bool NXObject::IsTransformType()
{
	return m_type == NXType::eCamera ||
		m_type == NXType::ePrefab ||
		m_type == NXType::ePrimitive ||
		m_type == NXType::eCubeMap;
}

void NXObject::AddScript(NXScript* script)
{
	script->SetObject(this);
	m_scripts.push_back(script);
}

std::vector<NXScript*> NXObject::GetScripts()
{
	return m_scripts;
}

NXObject* NXObject::GetParent()
{
	return m_parent;
}

void NXObject::SetParent(NXObject* pParent)
{
	if (m_parent)
		m_parent->RemoveChild(this);

	// 不考虑SetParent为nullptr的情况，因为场景中始终有一个pRootObject。
	m_parent = pParent;
	pParent->m_childs.push_back(this);
}

size_t NXObject::GetChildCount()
{
	return m_childs.size();
}

std::list<NXObject*> NXObject::GetChilds()
{
	return m_childs;
}

void NXObject::RemoveChild(NXObject* pObject)
{
	m_childs.remove(pObject);
}

bool NXObject::IsChild(NXObject* pObject)
{
	auto pParent = pObject->m_parent;
	while (pParent)
	{
		if (pParent == this) return true;
		pParent = pParent->m_parent;
	}
	return false;
}

void NXObject::Update()
{
	for (auto script : m_scripts)
	{
		script->Update();
	}
}

void NXObject::Release()
{
	for (auto script : m_scripts)
	{
		SafeDelete(script);
	}

	for (auto pChild : m_childs)
	{
		SafeRelease(pChild);
	}
}