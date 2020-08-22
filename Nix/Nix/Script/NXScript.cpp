#include "NXScript.h"

NXScript::~NXScript()
{
}

void NXScript::SetObject(const std::shared_ptr<NXObject>& pObject)
{
	m_pObject = pObject;
}
