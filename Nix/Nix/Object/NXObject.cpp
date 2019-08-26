#include "NXObject.h"

string NXObject::GetName()
{
	return m_name;
}

void NXObject::SetName(string name)
{
	m_name = name;
}
