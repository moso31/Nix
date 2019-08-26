#pragma once
#include "Header.h"

class NXObject
{
public:
	NXObject() = default;
	virtual ~NXObject() {}

	string GetName();
	void SetName(string name);

protected:
	string m_name;
};