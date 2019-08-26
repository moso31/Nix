#pragma once
#include "Header.h"

class NXObject
{
public:
	NXObject() = default;
	string GetName();
	void SetName(string name);

protected:
	string m_name;
};