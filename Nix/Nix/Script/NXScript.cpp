#include "NXScript.h"

NXScript::~NXScript()
{
}

void NXScript::SetObject(const shared_ptr<NXObject>& pObject)
{
	m_pObject = pObject;
}
