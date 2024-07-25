#include "NXScript.h"
#include "NXScriptable.h"

NXScript::~NXScript()
{
}

void NXScript::SetObject(NXScriptable* pObject)
{
	m_pObject = pObject;
}

void NXScript::Release()
{
	if (m_pObject)
	{
		m_pObject->Destroy();
		SafeDelete(m_pObject);
	}
}
