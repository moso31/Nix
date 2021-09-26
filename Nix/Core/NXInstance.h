#pragma once
#include "Header.h"

class NXInstance
{
public:
	NXInstance() {}
	~NXInstance() {}

	static NXInstance* GetInstance()
	{
		if (!pInstance) pInstance = new NXInstance();
		return pInstance;
	}

	static NXInstance* pInstance;
};
