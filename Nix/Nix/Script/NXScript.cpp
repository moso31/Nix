#include "NXScript.h"

NXScript::~NXScript()
{
}

void NXScript::SetObject(NXScriptable* pObject)
{
	m_pObject = pObject;
}
