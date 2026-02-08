#include "BaseDefs/NixCore.h"
#include "NXObject.h"

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

void NXObject::Release()
{
	for (auto pChild : m_childs)
	{
		SafeRelease(pChild);
	}
}